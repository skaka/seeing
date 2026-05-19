///////////////////////////////////////////////////////////////////////////////
/// @file   timeline_clip_item.cpp
/// @brief  Custom-painted clip rectangle with name, duration, and decoration.
///////////////////////////////////////////////////////////////////////////////

#include "timeline_clip_item.h"

#include <QPen>
#include <QBrush>
#include <QFont>
#include <QLinearGradient>
#include <cmath>

namespace Seeing {

TimelineClipItem::TimelineClipItem(const QString& clipId,
                                   const QString& name,
                                   double timelineStart,
                                   double timelineEnd,
                                   int trackIndex,
                                   const QColor& color,
                                   double pixelsPerSecond,
                                   double trackHeight,
                                   QGraphicsItem* parent)
    : QGraphicsRectItem(parent)
    , m_clipId(clipId)
    , m_name(name)
    , m_timelineStart(timelineStart)
    , m_timelineEnd(timelineEnd)
    , m_trackIndex(trackIndex)
    , m_color(color)
{
    double x = timelineStart * pixelsPerSecond;
    double y = trackIndex * (trackHeight + 4) + 30;  // 30px header offset
    double w = (timelineEnd - timelineStart) * pixelsPerSecond;
    double h = trackHeight;

    setRect(x, y, w, h);
    setToolTip(QString("%1\n%2s → %3s\nTrack %4")
                   .arg(name)
                   .arg(timelineStart, 0, 'f', 1)
                   .arg(timelineEnd, 0, 'f', 1)
                   .arg(trackIndex + 1));
}

void TimelineClipItem::paint(QPainter* painter,
                              const QStyleOptionGraphicsItem* /*option*/,
                              QWidget* /*widget*/)
{
    QRectF r = rect();
    painter->setRenderHint(QPainter::Antialiasing, true);

    // ── Background gradient ─────────────────────────────────────────────────
    QLinearGradient grad(r.topLeft(), r.bottomLeft());
    grad.setColorAt(0.0, m_color.lighter(120));
    grad.setColorAt(0.4, m_color);
    grad.setColorAt(1.0, m_color.darker(130));
    painter->setBrush(grad);
    painter->setPen(QPen(m_color.darker(150), 1.0));
    painter->drawRoundedRect(r, 4, 4);

    // ── Waveform-like decoration ────────────────────────────────────────────
    {
        painter->save();
        painter->setClipRect(r);
        QPen wavePen(QColor(255, 255, 255, 40), 1.0);
        painter->setPen(wavePen);

        double centerY = r.center().y();
        double amplitude = r.height() * 0.25;
        double step = 3.0;

        for (double x = r.left() + 4; x < r.right() - 4; x += step) {
            // Pseudo-random waveform using sin
            double t = (x - r.left()) * 0.15;
            double h1 = std::sin(t) * amplitude * 0.7;
            double h2 = std::sin(t * 2.3 + 1.0) * amplitude * 0.4;
            double h = h1 + h2;
            painter->drawLine(QPointF(x, centerY - std::abs(h)),
                              QPointF(x, centerY + std::abs(h)));
        }
        painter->restore();
    }

    // ── Clip name text ──────────────────────────────────────────────────────
    painter->setPen(QColor(255, 255, 255, 220));
    QFont font;
    font.setFamilies({"Inter", "SF Pro Display", "Segoe UI", "Noto Sans", "sans-serif"});
    font.setPointSize(9);
    font.setWeight(QFont::DemiBold);
    painter->setFont(font);

    QRectF textRect = r.adjusted(8, 4, -8, -r.height() / 2);
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, m_name);

    // ── Duration label ──────────────────────────────────────────────────────
    painter->setPen(QColor(255, 255, 255, 120));
    QFont smallFont;
    smallFont.setFamilies({"Inter", "SF Pro Display", "Segoe UI", "Noto Sans", "sans-serif"});
    smallFont.setPointSize(7);
    painter->setFont(smallFont);

    double duration = m_timelineEnd - m_timelineStart;
    QString durText = QString("%1s").arg(duration, 0, 'f', 1);
    QRectF durRect = r.adjusted(8, r.height() / 2, -8, -4);
    painter->drawText(durRect, Qt::AlignRight | Qt::AlignVCenter, durText);

    // ── Left/Right trim handles ─────────────────────────────────────────────
    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(255, 255, 255, 50));
    painter->drawRoundedRect(QRectF(r.left(), r.top() + 2, 3, r.height() - 4), 1, 1);
    painter->drawRoundedRect(QRectF(r.right() - 3, r.top() + 2, 3, r.height() - 4), 1, 1);
}

} // namespace Seeing
