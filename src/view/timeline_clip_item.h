///////////////////////////////////////////////////////////////////////////////
/// @file   timeline_clip_item.h
/// @brief  A single clip rendered as a QGraphicsRectItem on the timeline.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QString>
#include <QColor>

namespace Seeing {

///////////////////////////////////////////////////////////////////////////////
/// @class TimelineClipItem
/// @brief Visual representation of a clip on the timeline.
///
/// Each clip is a styled rectangle with a name label, duration info,
/// and waveform-like decoration. It passively reflects Model data.
///////////////////////////////////////////////////////////////////////////////
class TimelineClipItem : public QGraphicsRectItem
{
public:
    TimelineClipItem(const QString& clipId,
                     const QString& name,
                     double timelineStart,
                     double timelineEnd,
                     int trackIndex,
                     const QColor& color,
                     double pixelsPerSecond,
                     double trackHeight,
                     QGraphicsItem* parent = nullptr);

    QString clipId() const { return m_clipId; }

    /// Custom paint for the clip appearance.
    void paint(QPainter* painter,
               const QStyleOptionGraphicsItem* option,
               QWidget* widget) override;

private:
    QString m_clipId;
    QString m_name;
    double  m_timelineStart;
    double  m_timelineEnd;
    int     m_trackIndex;
    QColor  m_color;
};

} // namespace Seeing
