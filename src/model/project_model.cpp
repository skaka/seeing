///////////////////////////////////////////////////////////////////////////////
/// @file   project_model.cpp
/// @brief  Implementation of the Model layer.
///////////////////////////////////////////////////////////////////////////////

#include "project_model.h"
#include <QJsonDocument>
#include <algorithm>

namespace Seeing {

// ═══════════════════════════════════════════════════════════════════════════
//  ClipInfo  ─  JSON serialization
// ═══════════════════════════════════════════════════════════════════════════

QJsonObject ClipInfo::toJson() const
{
    QJsonObject obj;
    obj["id"]             = id;
    obj["asset_id"]       = assetId;
    obj["name"]           = name;
    obj["track_index"]    = trackIndex;
    obj["timeline_start"] = timelineStart;
    obj["timeline_end"]   = timelineEnd;
    obj["media_in"]       = mediaIn;
    obj["media_out"]      = mediaOut;
    obj["color"]          = color.name();
    return obj;
}

ClipInfo ClipInfo::fromJson(const QJsonObject& obj)
{
    ClipInfo c;
    c.id            = obj["id"].toString();
    c.assetId       = obj["asset_id"].toString();
    c.name          = obj["name"].toString();
    c.trackIndex    = obj["track_index"].toInt();
    c.timelineStart = obj["timeline_start"].toDouble();
    c.timelineEnd   = obj["timeline_end"].toDouble();
    c.mediaIn       = obj["media_in"].toDouble();
    c.mediaOut      = obj["media_out"].toDouble();
    c.color         = QColor(obj["color"].toString("#4fc3f7"));
    return c;
}

// ═══════════════════════════════════════════════════════════════════════════
//  AssetInfo  ─  JSON serialization
// ═══════════════════════════════════════════════════════════════════════════

QJsonObject AssetInfo::toJson() const
{
    QJsonObject obj;
    obj["id"]          = id;
    obj["file_path"]   = filePath;
    obj["name"]        = name;
    obj["duration"]    = duration;
    obj["type"]        = type;
    obj["description"] = description;
    return obj;
}

AssetInfo AssetInfo::fromJson(const QJsonObject& obj)
{
    AssetInfo a;
    a.id          = obj["id"].toString();
    a.filePath    = obj["file_path"].toString();
    a.name        = obj["name"].toString();
    a.duration    = obj["duration"].toDouble();
    a.type        = obj["type"].toString();
    a.description = obj["description"].toString();
    return a;
}

// ═══════════════════════════════════════════════════════════════════════════
//  ProjectModel
// ═══════════════════════════════════════════════════════════════════════════

ProjectModel::ProjectModel(QObject* parent)
    : QObject(parent)
{
}

// ── JSON I/O ────────────────────────────────────────────────────────────────

QJsonDocument ProjectModel::toJsonDocument() const
{
    QJsonObject root;

    // Project metadata
    QJsonObject meta;
    meta["version"]      = "0.1.0";
    meta["track_count"]  = m_trackCount;
    meta["playhead_pos"] = m_playheadPos;
    root["project"]      = meta;

    // Assets array
    QJsonArray assetsArr;
    for (const auto& a : m_assets)
        assetsArr.append(a.toJson());
    root["assets"] = assetsArr;

    // Clips array
    QJsonArray clipsArr;
    for (const auto& c : m_clips)
        clipsArr.append(c.toJson());
    root["clips"] = clipsArr;

    return QJsonDocument(root);
}

void ProjectModel::loadFromJson(const QJsonDocument& doc)
{
    QJsonObject root = doc.object();

    // Project metadata
    QJsonObject meta = root["project"].toObject();
    m_trackCount  = meta["track_count"].toInt(3);
    m_playheadPos = meta["playhead_pos"].toDouble(0.0);

    // Assets
    m_assets.clear();
    QJsonArray assetsArr = root["assets"].toArray();
    for (const auto& val : assetsArr)
        m_assets.append(AssetInfo::fromJson(val.toObject()));

    // Clips
    m_clips.clear();
    QJsonArray clipsArr = root["clips"].toArray();
    for (const auto& val : clipsArr)
        m_clips.append(ClipInfo::fromJson(val.toObject()));

    emitChange();
}

QString ProjectModel::toJsonString() const
{
    return QString::fromUtf8(toJsonDocument().toJson(QJsonDocument::Indented));
}

// ── Assets ──────────────────────────────────────────────────────────────────

void ProjectModel::addAsset(const AssetInfo& asset)
{
    pushUndoSnapshot();
    m_assets.append(asset);
    emit assetAdded(asset.id);
    emitChange();
}

void ProjectModel::removeAsset(const QString& assetId)
{
    pushUndoSnapshot();
    m_assets.erase(
        std::remove_if(m_assets.begin(), m_assets.end(),
                       [&](const AssetInfo& a) { return a.id == assetId; }),
        m_assets.end());
    emitChange();
}

void ProjectModel::updateAssetDescription(const QString& assetId, const QString& description)
{
    for (auto& a : m_assets) {
        if (a.id == assetId) {
            a.description = description;
            emitChange();
            break;
        }
    }
}

QVector<AssetInfo> ProjectModel::assets() const { return m_assets; }

AssetInfo ProjectModel::asset(const QString& assetId) const
{
    for (const auto& a : m_assets)
        if (a.id == assetId) return a;
    return {};
}

// ── Clips ───────────────────────────────────────────────────────────────────

void ProjectModel::addClip(const ClipInfo& clip)
{
    pushUndoSnapshot();
    m_clips.append(clip);
    emit timelineChanged();
    emitChange();
}

void ProjectModel::removeClip(const QString& clipId)
{
    pushUndoSnapshot();
    m_clips.erase(
        std::remove_if(m_clips.begin(), m_clips.end(),
                       [&](const ClipInfo& c) { return c.id == clipId; }),
        m_clips.end());
    emit timelineChanged();
    emitChange();
}

void ProjectModel::updateClip(const ClipInfo& clip)
{
    pushUndoSnapshot();
    for (auto& c : m_clips) {
        if (c.id == clip.id) {
            c = clip;
            break;
        }
    }
    emit timelineChanged();
    emitChange();
}

QVector<ClipInfo> ProjectModel::clips() const { return m_clips; }

ClipInfo ProjectModel::clip(const QString& clipId) const
{
    for (const auto& c : m_clips)
        if (c.id == clipId) return c;
    return {};
}

// ── Timeline Metadata ───────────────────────────────────────────────────────

int  ProjectModel::trackCount() const           { return m_trackCount; }
void ProjectModel::setTrackCount(int count)      { m_trackCount = count; emitChange(); }

double ProjectModel::playheadPosition() const    { return m_playheadPos; }
void   ProjectModel::setPlayheadPosition(double s) { m_playheadPos = s; /* no full redraw */ }

// ── Undo / Redo ─────────────────────────────────────────────────────────────

void ProjectModel::pushUndoSnapshot()
{
    m_undoStack.append(toJsonDocument());
    if (m_undoStack.size() > kMaxUndoDepth)
        m_undoStack.removeFirst();
    m_redoStack.clear();
    emit undoRedoStateChanged(canUndo(), canRedo());
}

bool ProjectModel::canUndo() const { return !m_undoStack.isEmpty(); }
bool ProjectModel::canRedo() const { return !m_redoStack.isEmpty(); }

void ProjectModel::undo()
{
    if (!canUndo()) return;
    m_redoStack.append(toJsonDocument());  // save current for redo
    QJsonDocument prev = m_undoStack.takeLast();
    loadFromJson(prev);   // loadFromJson calls emitChange()
    emit undoRedoStateChanged(canUndo(), canRedo());
}

void ProjectModel::redo()
{
    if (!canRedo()) return;
    m_undoStack.append(toJsonDocument());  // save current for undo
    QJsonDocument next = m_redoStack.takeLast();
    loadFromJson(next);
    emit undoRedoStateChanged(canUndo(), canRedo());
}

// ── Private ─────────────────────────────────────────────────────────────────

void ProjectModel::emitChange()
{
    emit modelChanged();
}

} // namespace Seeing
