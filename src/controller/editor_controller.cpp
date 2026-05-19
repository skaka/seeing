///////////////////////////////////////////////////////////////////////////////
/// @file   editor_controller.cpp
/// @brief  Implementation of the Controller layer.
///
/// Core workflow:
///   User prompt → AI Engine → JSON mutation → Model update → View redraws
///////////////////////////////////////////////////////////////////////////////

#include "editor_controller.h"

#include <QJsonObject>
#include <QUuid>
#include <QFileInfo>
#include <algorithm>

namespace Seeing {

EditorController::EditorController(ProjectModel& model, QObject* parent)
    : QObject(parent)
    , m_model(model)
    , m_aiEngine(AiEngineFactory::create(AiEngineFactory::EngineType::Dummy, this))
{
    // Forward AI status messages to the chat.
    connect(m_aiEngine.get(), &AiEngineInterface::statusMessage,
            this, [this](const QString& msg) {
                emit chatResponse("AI", msg);
            });
}

// ── Chat prompt handling ────────────────────────────────────────────────────

void EditorController::handleChatPrompt(const QString& prompt)
{
    // 1. Echo the user's message
    emit chatResponse("You", prompt);

    // 2. Get current project state for context
    QJsonObject projectState = m_model.toJsonDocument().object();

    // 3. Route to AI engine → get mutation command
    QJsonObject mutation = m_aiEngine->processPrompt(prompt, projectState);

    // 4. Apply the mutation to the Model
    applyMutationCommand(mutation);
}

// ── Mutation application ────────────────────────────────────────────────────

void EditorController::applyMutationCommand(const QJsonObject& command)
{
    QString action = command["action"].toString();
    QJsonObject params = command["params"].toObject();

    if (action == "add_clip") {
        ClipInfo clip;
        clip.id            = params["id"].toString(
                                QUuid::createUuid().toString(QUuid::WithoutBraces));
        clip.name          = params["name"].toString("Untitled");
        clip.trackIndex    = params["track_index"].toInt(0);
        clip.timelineStart = params["timeline_start"].toDouble(0.0);
        clip.timelineEnd   = params["timeline_end"].toDouble(5.0);
        clip.mediaIn       = params["media_in"].toDouble(0.0);
        clip.mediaOut      = params["media_out"].toDouble(5.0);
        clip.color         = QColor(params["color"].toString("#4fc3f7"));
        m_model.addClip(clip);

        emit chatResponse("System",
            QString("📎 Clip '%1' added to Track %2 [%3s → %4s].")
                .arg(clip.name)
                .arg(clip.trackIndex + 1)
                .arg(clip.timelineStart)
                .arg(clip.timelineEnd));
    }
    else if (action == "remove_clip") {
        QString targetName = params["name"].toString();
        auto clips = m_model.clips();
        for (const auto& c : clips) {
            if (c.name.toLower() == targetName.toLower()) {
                m_model.removeClip(c.id);
                emit chatResponse("System",
                    QString("🗑️ Clip '%1' removed.").arg(c.name));
                return;
            }
        }
        emit chatResponse("System",
            QString("⚠️ No clip named '%1' found.").arg(targetName));
    }
    else if (action == "trim_clip") {
        QString targetName = params["name"].toString();
        double newDuration = params["duration"].toDouble();
        auto clips = m_model.clips();
        for (auto c : clips) {
            if (c.name.toLower() == targetName.toLower()) {
                c.timelineEnd = c.timelineStart + newDuration;
                c.mediaOut    = c.mediaIn + newDuration;
                m_model.updateClip(c);
                emit chatResponse("System",
                    QString("✂️ Clip '%1' trimmed to %2s.")
                        .arg(c.name).arg(newDuration));
                return;
            }
        }
        emit chatResponse("System",
            QString("⚠️ No clip named '%1' found.").arg(targetName));
    }
    else if (action == "trim_first_clip") {
        double cutSeconds = params["cut_seconds"].toDouble();
        auto clips = m_model.clips();
        if (!clips.isEmpty()) {
            // Find the clip with the earliest start time.
            auto it = std::min_element(clips.begin(), clips.end(),
                [](const ClipInfo& a, const ClipInfo& b) {
                    return a.timelineStart < b.timelineStart;
                });
            ClipInfo c = *it;
            double originalDuration = c.timelineEnd - c.timelineStart;
            if (cutSeconds >= originalDuration) {
                m_model.removeClip(c.id);
                emit chatResponse("System",
                    QString("🗑️ Clip '%1' was shorter than %2s — removed entirely.")
                        .arg(c.name).arg(cutSeconds));
            } else {
                c.timelineStart += cutSeconds;
                c.mediaIn       += cutSeconds;
                m_model.updateClip(c);
                emit chatResponse("System",
                    QString("✂️ Cut %1s from the beginning of '%2'. New range: [%3s → %4s].")
                        .arg(cutSeconds).arg(c.name)
                        .arg(c.timelineStart).arg(c.timelineEnd));
            }
        } else {
            emit chatResponse("System", "⚠️ No clips on the timeline to trim.");
        }
    }
    else if (action == "move_clip") {
        QString targetName = params["name"].toString();
        double newPos = params["position"].toDouble();
        auto clips = m_model.clips();
        for (auto c : clips) {
            if (c.name.toLower() == targetName.toLower()) {
                double duration = c.timelineEnd - c.timelineStart;
                c.timelineStart = newPos;
                c.timelineEnd   = newPos + duration;
                m_model.updateClip(c);
                emit chatResponse("System",
                    QString("➡️ Clip '%1' moved to %2s.")
                        .arg(c.name).arg(newPos));
                return;
            }
        }
        emit chatResponse("System",
            QString("⚠️ No clip named '%1' found.").arg(targetName));
    }
    else if (action == "undo") {
        m_model.undo();
        emit chatResponse("System", "↩️ Undo performed.");
    }
    else if (action == "redo") {
        m_model.redo();
        emit chatResponse("System", "↪️ Redo performed.");
    }
    else if (action == "help") {
        // Status message already emitted by the engine.
    }
    else if (action == "unknown") {
        // Error already emitted by the engine.
    }
}

// ── AI engine switching ─────────────────────────────────────────────────────

void EditorController::setAiEngine(AiEngineFactory::EngineType type)
{
    m_aiEngine = AiEngineFactory::create(type, this);
    connect(m_aiEngine.get(), &AiEngineInterface::statusMessage,
            this, [this](const QString& msg) {
                emit chatResponse("AI", msg);
            });
    emit engineChanged(m_aiEngine->engineName());
    emit chatResponse("System",
        QString("🔄 AI Engine switched to: %1").arg(m_aiEngine->engineName()));
}

void EditorController::setAiEngine(const QString& engineName)
{
    m_aiEngine = AiEngineFactory::create(engineName, this);
    connect(m_aiEngine.get(), &AiEngineInterface::statusMessage,
            this, [this](const QString& msg) {
                emit chatResponse("AI", msg);
            });
    emit engineChanged(m_aiEngine->engineName());
    emit chatResponse("System",
        QString("🔄 AI Engine switched to: %1").arg(m_aiEngine->engineName()));
}

QString EditorController::currentEngineName() const
{
    return m_aiEngine ? m_aiEngine->engineName() : "None";
}

// ── Direct model manipulation ───────────────────────────────────────────────

void EditorController::addAssetFromFile(const QString& filePath)
{
    QFileInfo fi(filePath);
    AssetInfo asset;
    asset.id       = QUuid::createUuid().toString(QUuid::WithoutBraces);
    asset.filePath = filePath;
    asset.name     = fi.baseName();
    asset.duration = 30.0;   // Simulated — real FFmpeg probe goes here later
    asset.type     = "video"; // Simulated

    m_model.addAsset(asset);
    emit chatResponse("System",
        QString("📁 Asset '%1' imported (simulated: 30s video).").arg(asset.name));
}

void EditorController::removeAsset(const QString& assetId)
{
    m_model.removeAsset(assetId);
}

void EditorController::undo() { m_model.undo(); }
void EditorController::redo() { m_model.redo(); }

} // namespace Seeing
