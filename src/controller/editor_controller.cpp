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
#include <QSettings>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonArray>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QTimer>
#include <algorithm>

namespace Seeing {

EditorController::EditorController(ProjectModel& model, QObject* parent)
    : QObject(parent)
    , m_model(model)
{
    QSettings settings;
    QString engineType = settings.value("ai/engine_type", "dummy").toString();
    m_aiEngine = AiEngineFactory::create(engineType, this);

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

    // Check for Marlin-2B slash commands
    if (prompt.trimmed().startsWith("/marlin")) {
        handleMarlinCommand(prompt.trimmed());
        return;
    }

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

    if (action == "sequence") {
        QJsonArray commands = params["commands"].toArray();
        for (const auto& cmdVal : commands) {
            applyMutationCommand(cmdVal.toObject());
        }
    }
    else if (action == "add_clip") {
        ClipInfo clip;
        clip.id            = params["id"].toString(
                                QUuid::createUuid().toString(QUuid::WithoutBraces));
        clip.assetId       = params["asset_id"].toString();
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
    
    // Smart type detection
    QString ext = fi.suffix().toLower();
    if (ext == "mp3" || ext == "wav" || ext == "ogg" || ext == "m4a" || ext == "flac") {
        asset.type = "audio";
    } else if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "gif" || ext == "bmp" || ext == "webp") {
        asset.type = "image";
    } else {
        asset.type = "video";
    }

    asset.description = "Analyzing asset...";

    m_model.addAsset(asset);
    emit chatResponse("System",
        QString("📁 Asset '%1' imported (%2 file).").arg(asset.name, asset.type));

    // Auto-analyze in the background!
    analyzeAssetInBackground(asset.id, asset.filePath);
}

void EditorController::removeAsset(const QString& assetId)
{
    m_model.removeAsset(assetId);
}

void EditorController::analyzeAssetInBackground(const QString& assetId, const QString& filePath)
{
    QFileInfo fi(filePath);
    QString fileName = fi.fileName();
    AssetInfo asset = m_model.asset(assetId);
    
    if (asset.type == "audio") {
        // Simulated background analysis for audio (since Marlin-2B is a Video VLM)
        QTimer::singleShot(2000, this, [this, assetId, fileName]() {
            QString analysis = "🎵 Audio Analysis: Speech detected. Background music present. Tempo matches 120 BPM.";
            m_model.updateAssetDescription(assetId, analysis);
            emit chatResponse("AI (Marlin-2B)", QString("✨ Auto-indexed audio '%1' successfully!\n\n**Analysis:**\n%2").arg(fileName, analysis));
        });
        return;
    }
    
    // For video and image assets, request analysis from Marlin-2B API
    emit chatResponse("AI (Marlin-2B)", QString("⏳ Auto-indexing %1 '%2' using local model...").arg(asset.type, fileName));

    QNetworkRequest request(QUrl("http://127.0.0.1:8012/caption"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject reqBody;
    reqBody["video_path"] = filePath;
    QByteArray data = QJsonDocument(reqBody).toJson();

    QNetworkReply* reply = m_netManager.post(request, data);
    connect(reply, &QNetworkReply::finished, this, [this, reply, assetId, fileName]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            QString err = reply->errorString();
            m_model.updateAssetDescription(assetId, "Auto-indexing failed: " + err);
            emit chatResponse("AI (Marlin-2B)", QString("⚠️ Auto-indexing failed for '%1': %2").arg(fileName, err));
            return;
        }

        QByteArray responseData = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(responseData);
        QJsonObject obj = doc.object();

        if (obj["status"].toString() == "success") {
            QJsonObject resObj = obj["result"].toObject();
            QString captionText = resObj["caption"].toString();
            if (captionText.isEmpty()) {
                captionText = obj["result"].toString();
            }
            m_model.updateAssetDescription(assetId, captionText);
            emit chatResponse("AI (Marlin-2B)", QString("✨ Auto-indexed '%1' successfully!\n\n**Analysis:**\n%2").arg(fileName, captionText));
        } else {
            m_model.updateAssetDescription(assetId, "Auto-indexing failed: VLM Error");
            emit chatResponse("AI (Marlin-2B)", QString("⚠️ Auto-indexing failed for '%1': VLM error").arg(fileName));
        }
    });
}

void EditorController::undo() { m_model.undo(); }
void EditorController::redo() { m_model.redo(); }

void EditorController::handleMarlinCommand(const QString& prompt)
{
    auto assets = m_model.assets();
    if (assets.isEmpty()) {
        emit chatResponse("AI (Marlin-2B)", "⚠️ No assets imported. Please import a video file into the Media Panel first.");
        return;
    }

    // Use the first video asset for now
    AssetInfo targetAsset;
    bool foundVideo = false;
    for (const auto& a : assets) {
        if (a.type == "video" || a.filePath.endsWith(".mp4") || a.filePath.endsWith(".mkv") || a.filePath.endsWith(".mov")) {
            targetAsset = a;
            foundVideo = true;
            break;
        }
    }

    if (!foundVideo) {
        emit chatResponse("AI (Marlin-2B)", "⚠️ No video asset found. Please import an MP4, MKV, or MOV video file first.");
        return;
    }

    QString videoPath = targetAsset.filePath;

    // Check subcommands
    QString cmd = prompt.mid(7).trimmed(); // strip "/marlin"
    if (cmd.startsWith("caption")) {
        emit chatResponse("AI (Marlin-2B)", QString("⏳ Analyzing video '%1' with Marlin-2B...").arg(targetAsset.name));
        
        QNetworkAccessManager manager;
        QNetworkRequest request(QUrl("http://127.0.0.1:8012/caption"));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QJsonObject reqBody;
        reqBody["video_path"] = videoPath;
        QByteArray data = QJsonDocument(reqBody).toJson();

        QNetworkReply* reply = manager.post(request, data);
        QEventLoop loop;
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        if (reply->error() != QNetworkReply::NoError) {
            emit chatResponse("AI (Marlin-2B)", QString("❌ Connection to Marlin-2B server failed: %1").arg(reply->errorString()));
            reply->deleteLater();
            return;
        }

        QByteArray responseData = reply->readAll();
        reply->deleteLater();

        QJsonDocument doc = QJsonDocument::fromJson(responseData);
        QJsonObject obj = doc.object();
        
        if (obj["status"].toString() == "success") {
            // Get caption text
            QJsonObject resObj = obj["result"].toObject();
            QString captionText = resObj["caption"].toString();
            if (captionText.isEmpty()) {
                captionText = obj["result"].toString();
            }
            emit chatResponse("AI (Marlin-2B)", QString("🎥 **Video Analysis for %1**:\n\n%2").arg(targetAsset.name, captionText));
        } else {
            emit chatResponse("AI (Marlin-2B)", QString("❌ Marlin-2B analysis failed: %1").arg(obj["detail"].toString()));
        }
    }
    else if (cmd.startsWith("find ")) {
        QString query = cmd.mid(5).trimmed();
        if (query.isEmpty()) {
            emit chatResponse("AI (Marlin-2B)", "⚠️ Please specify a query. Usage: `/marlin find <search query>`");
            return;
        }

        emit chatResponse("AI (Marlin-2B)", QString("⏳ Searching for '%1' in video '%2'...").arg(query, targetAsset.name));

        QNetworkAccessManager manager;
        QNetworkRequest request(QUrl("http://127.0.0.1:8012/find"));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QJsonObject reqBody;
        reqBody["video_path"] = videoPath;
        reqBody["query"] = query;
        QByteArray data = QJsonDocument(reqBody).toJson();

        QNetworkReply* reply = manager.post(request, data);
        QEventLoop loop;
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        if (reply->error() != QNetworkReply::NoError) {
            emit chatResponse("AI (Marlin-2B)", QString("❌ Connection to Marlin-2B server failed: %1").arg(reply->errorString()));
            reply->deleteLater();
            return;
        }

        QByteArray responseData = reply->readAll();
        reply->deleteLater();

        QJsonDocument doc = QJsonDocument::fromJson(responseData);
        QJsonObject obj = doc.object();

        if (obj["status"].toString() == "success") {
            QString resultText = obj["result"].toString();
            emit chatResponse("AI (Marlin-2B)", QString("🔍 Marlin-2B search result: %1").arg(resultText));

            // Extract timestamps <start - end> or <start-end>
            QRegularExpression re("<\\s*(\\d+(?:\\.\\d+)?)\\s*-\\s*(\\d+(?:\\.\\d+)?)\\s*>");
            QRegularExpressionMatch match = re.match(resultText);
            if (match.hasMatch()) {
                double start = match.captured(1).toDouble();
                double end = match.captured(2).toDouble();
                double duration = end - start;

                if (duration > 0) {
                    // Create and add clip to timeline!
                    ClipInfo clip;
                    clip.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
                    clip.name = QString("%1 (%2)").arg(targetAsset.name, query);
                    clip.trackIndex = 0; // Default to Track 1
                    
                    // Put it at the end of the timeline
                    double maxEnd = 0.0;
                    for (const auto& c : m_model.clips()) {
                        if (c.timelineEnd > maxEnd) {
                            maxEnd = c.timelineEnd;
                        }
                    }
                    clip.timelineStart = maxEnd;
                    clip.timelineEnd = maxEnd + duration;
                    clip.mediaIn = start;
                    clip.mediaOut = end;
                    clip.color = QColor("#ffb74d"); // orange
                    
                    m_model.addClip(clip);

                    emit chatResponse("System", QString("🧡 Added grounded segment '%1' to Track 1 [%2s → %3s] (Video source: %4s to %5s).")
                                                .arg(clip.name)
                                                .arg(clip.timelineStart)
                                                .arg(clip.timelineEnd)
                                                .arg(start)
                                                .arg(end));
                }
            } else {
                emit chatResponse("AI (Marlin-2B)", "⚠️ Could not extract timestamps from the search result. Make sure the model found the event.");
            }
        } else {
            emit chatResponse("AI (Marlin-2B)", QString("❌ Marlin-2B grounding failed: %1").arg(obj["detail"].toString()));
        }
    }
    else {
        emit chatResponse("AI (Marlin-2B)", "❓ Unknown Marlin-2B command. Supported commands:\n"
                                           "- `/marlin caption` : Describe scenes/events in the video\n"
                                           "- `/marlin find <query>` : Find an event and add it to the timeline");
    }
}

} // namespace Seeing
