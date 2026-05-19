///////////////////////////////////////////////////////////////////////////////
/// @file   timeline_panel.cpp
/// @brief  Multi-track timeline built with QGraphicsScene/QGraphicsView.
///
/// The timeline is a PASSIVE view — it only reads from the Model and
/// rebuilds when modelChanged() is emitted. It never mutates the Model.
///////////////////////////////////////////////////////////////////////////////

#include "timeline_panel.h"
#include "timeline_clip_item.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGraphicsRectItem>
#include <QGraphicsLineItem>
#include <QGraphicsSimpleTextItem>
#include <QPen>
#include <QBrush>
#include <QFont>
#include <QScrollBar>

namespace Seeing {

TimelinePanel::TimelinePanel(ProjectModel& model, QWidget* parent)
    : QWidget(parent)
    , m_model(model)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // ── Header ──────────────────────────────────────────────────────────────
    m_headerLabel = new QLabel("  TIMELINE", this);
    m_headerLabel->setFixedHeight(static_cast<int>(kHeaderHeight));
    m_headerLabel->setStyleSheet(
        "background: #252526; color: #888888; font-size: 11px;"
        "font-weight: bold; letter-spacing: 1px;"
        "border-bottom: 1px solid #3c3c3c; padding-left: 8px;");
    layout->addWidget(m_headerLabel);

    // ── Graphics Scene & View ───────────────────────────────────────────────
    m_scene = new QGraphicsScene(this);
    m_scene->setBackgroundBrush(QColor(30, 30, 30));

    m_view = new QGraphicsView(m_scene, this);
    m_view->setRenderHint(QPainter::Antialiasing, true);
    m_view->setRenderHint(QPainter::SmoothPixmapTransform, true);
    m_view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_view->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_view->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    m_view->setDragMode(QGraphicsView::ScrollHandDrag);
    m_view->setStyleSheet(
        "QGraphicsView { border: none; background: #1e1e1e; }");

    layout->addWidget(m_view, 1);

    // ── Connect to Model ────────────────────────────────────────────────────
    connect(&m_model, &ProjectModel::modelChanged,
            this, &TimelinePanel::rebuildTimeline);
    connect(&m_model, &ProjectModel::timelineChanged,
            this, &TimelinePanel::rebuildTimeline);

    // Initial draw
    rebuildTimeline();
}

void TimelinePanel::rebuildTimeline()
{
    m_scene->clear();

    int trackCount = m_model.trackCount();
    double totalHeight = kRulerHeight + trackCount * (kTrackHeight + 4) + 40;
    m_scene->setSceneRect(0, 0, kSceneWidth, totalHeight);

    drawTimeRuler();
    drawTrackBackgrounds();
    drawClips();
    drawPlayhead();

    // Update header info
    auto clips = m_model.clips();
    m_headerLabel->setText(
        QString("  TIMELINE  —  %1 track(s)  |  %2 clip(s)")
            .arg(trackCount)
            .arg(clips.size()));
}

void TimelinePanel::drawTimeRuler()
{
    // Ruler background
    m_scene->addRect(0, 0, kSceneWidth, kRulerHeight,
                     QPen(Qt::NoPen),
                     QBrush(QColor(37, 37, 38)));

    // Bottom border
    m_scene->addLine(0, kRulerHeight, kSceneWidth, kRulerHeight,
                     QPen(QColor(60, 60, 60), 1));

    // Time markers
    QFont rulerFont;
    rulerFont.setFamilies({"Inter", "SF Pro Display", "Segoe UI", "Noto Sans", "sans-serif"});
    rulerFont.setPointSize(8);
    double totalSeconds = kSceneWidth / kPixelsPerSecond;

    for (int sec = 0; sec <= static_cast<int>(totalSeconds); ++sec) {
        double x = sec * kPixelsPerSecond;

        if (sec % 5 == 0) {
            // Major tick + label
            auto* line = m_scene->addLine(x, 0, x, kRulerHeight,
                                           QPen(QColor(100, 100, 100), 1));
            Q_UNUSED(line);

            auto* label = m_scene->addSimpleText(
                QString("%1:%2")
                    .arg(sec / 60, 2, 10, QChar('0'))
                    .arg(sec % 60, 2, 10, QChar('0')),
                rulerFont);
            label->setBrush(QColor(150, 150, 150));
            label->setPos(x + 3, 2);
        } else {
            // Minor tick
            m_scene->addLine(x, kRulerHeight - 6, x, kRulerHeight,
                             QPen(QColor(70, 70, 70), 1));
        }
    }
}

void TimelinePanel::drawTrackBackgrounds()
{
    int trackCount = m_model.trackCount();
    QFont trackFont;
    trackFont.setFamilies({"Inter", "SF Pro Display", "Segoe UI", "Noto Sans", "sans-serif"});
    trackFont.setPointSize(9);
    trackFont.setWeight(QFont::DemiBold);

    for (int i = 0; i < trackCount; ++i) {
        double y = i * (kTrackHeight + 4) + kRulerHeight + 8;

        // Alternating track backgrounds
        QColor bgColor = (i % 2 == 0) ? QColor(28, 28, 28) : QColor(33, 33, 33);
        m_scene->addRect(0, y, kSceneWidth, kTrackHeight,
                         QPen(Qt::NoPen),
                         QBrush(bgColor));

        // Track separator line
        m_scene->addLine(0, y + kTrackHeight + 2,
                         kSceneWidth, y + kTrackHeight + 2,
                         QPen(QColor(45, 45, 45), 1));

        // Track label (left side)
        auto* label = m_scene->addSimpleText(
            QString("Track %1").arg(i + 1), trackFont);
        label->setBrush(QColor(80, 80, 80));
        label->setPos(6, y + (kTrackHeight - 14) / 2);
    }
}

void TimelinePanel::drawClips()
{
    const auto clips = m_model.clips();
    for (const auto& clip : clips) {
        auto* item = new TimelineClipItem(
            clip.id, clip.name,
            clip.timelineStart, clip.timelineEnd,
            clip.trackIndex, clip.color,
            kPixelsPerSecond, kTrackHeight);

        // Offset Y to account for ruler
        QRectF r = item->rect();
        r.moveTop(clip.trackIndex * (kTrackHeight + 4) + kRulerHeight + 8);
        item->setRect(r);

        m_scene->addItem(item);
    }
}

void TimelinePanel::drawPlayhead()
{
    double x = m_model.playheadPosition() * kPixelsPerSecond;
    double totalHeight = m_scene->sceneRect().height();

    // Playhead line
    auto* line = m_scene->addLine(x, 0, x, totalHeight,
                                   QPen(QColor(220, 50, 50), 2));
    line->setZValue(100);  // Always on top

    // Playhead handle (triangle at top)
    QPolygonF triangle;
    triangle << QPointF(x - 6, 0)
             << QPointF(x + 6, 0)
             << QPointF(x, 8);
    auto* handle = m_scene->addPolygon(triangle,
                                        QPen(Qt::NoPen),
                                        QBrush(QColor(220, 50, 50)));
    handle->setZValue(101);
}

} // namespace Seeing
