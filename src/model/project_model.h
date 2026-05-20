///////////////////////////////////////////////////////////////////////////////
/// @file   project_model.h
/// @brief  The MODEL layer — Master JSON state manager.
///
/// ProjectModel is the single source of truth for the entire editor.
/// It holds the project timeline, asset metadata, and sequence state as
/// an in-memory JSON object. All mutations go through this class, which
/// emits Qt signals so Views can reactively redraw.
///
/// Undo/Redo is implemented via snapshot history (like Git commits).
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QString>
#include <QVector>
#include <QUuid>
#include <QColor>

namespace Seeing {

/// Represents a single clip on the timeline.
struct ClipInfo {
    QString   id;              ///< Unique clip identifier
    QString   assetId;         ///< Reference to an asset in the media pool
    QString   name;            ///< Display name
    int       trackIndex = 0;  ///< Which track this clip lives on
    double    timelineStart = 0.0;  ///< Start position on the timeline (seconds)
    double    timelineEnd   = 0.0;  ///< End position on the timeline (seconds)
    double    mediaIn       = 0.0;  ///< In-point within the source media (seconds)
    double    mediaOut      = 0.0;  ///< Out-point within the source media (seconds)
    QColor    color;                ///< Display color

    QJsonObject toJson() const;
    static ClipInfo fromJson(const QJsonObject& obj);
};

/// Represents a single asset in the media pool.
struct AssetInfo {
    QString   id;              ///< Unique asset identifier
    QString   filePath;        ///< Original file path (never modified)
    QString   name;            ///< Display name
    double    duration = 0.0;  ///< Total duration in seconds
    QString   type;            ///< "video", "audio", "image"
    QString   description;     ///< VLM Analysis description

    QJsonObject toJson() const;
    static AssetInfo fromJson(const QJsonObject& obj);
};


///////////////////////////////////////////////////////////////////////////////
/// @class ProjectModel
/// @brief Central data store — reads/writes the Master JSON.
///////////////////////////////////////////////////////////////////////////////
class ProjectModel : public QObject
{
    Q_OBJECT

public:
    explicit ProjectModel(QObject* parent = nullptr);

    // ── JSON I/O ────────────────────────────────────────────────────────────
    /// Serialize the full project state to a JSON document.
    QJsonDocument toJsonDocument() const;
    /// Replace the full project state from a JSON document.
    void loadFromJson(const QJsonDocument& doc);
    /// Pretty-printed JSON string (for debugging / chat display).
    QString toJsonString() const;

    // ── Assets ──────────────────────────────────────────────────────────────
    void                   addAsset(const AssetInfo& asset);
    void                   removeAsset(const QString& assetId);
    void                   updateAssetDescription(const QString& assetId, const QString& description);
    QVector<AssetInfo>     assets() const;
    AssetInfo              asset(const QString& assetId) const;

    // ── Clips ───────────────────────────────────────────────────────────────
    void                   addClip(const ClipInfo& clip);
    void                   removeClip(const QString& clipId);
    void                   updateClip(const ClipInfo& clip);
    QVector<ClipInfo>      clips() const;
    ClipInfo               clip(const QString& clipId) const;

    // ── Timeline Metadata ───────────────────────────────────────────────────
    int    trackCount() const;
    void   setTrackCount(int count);
    double playheadPosition() const;
    void   setPlayheadPosition(double seconds);

    // ── Undo / Redo (snapshot-based) ────────────────────────────────────────
    void   pushUndoSnapshot();
    bool   canUndo() const;
    bool   canRedo() const;
    void   undo();
    void   redo();

signals:
    /// Emitted whenever the model state changes — Views must redraw.
    void modelChanged();
    /// Emitted when an asset is added to the pool.
    void assetAdded(const QString& assetId);
    /// Emitted when a clip is added/removed/modified.
    void timelineChanged();
    /// Emitted for undo/redo state updates.
    void undoRedoStateChanged(bool canUndo, bool canRedo);

private:
    /// Notify all views that state has mutated.
    void emitChange();

    // ── Data ────────────────────────────────────────────────────────────────
    QVector<AssetInfo>  m_assets;
    QVector<ClipInfo>   m_clips;
    int                 m_trackCount      = 3;
    double              m_playheadPos     = 0.0;

    // ── Undo history ────────────────────────────────────────────────────────
    QVector<QJsonDocument>  m_undoStack;
    QVector<QJsonDocument>  m_redoStack;
    static constexpr int    kMaxUndoDepth = 50;
};

} // namespace Seeing
