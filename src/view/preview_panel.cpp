///////////////////////////////////////////////////////////////////////////////
/// @file   preview_panel.cpp
/// @brief  Preview panel — displays a stylized placeholder + project info.
///////////////////////////////////////////////////////////////////////////////

#include "preview_panel.h"

#include <QVBoxLayout>
#include <QFont>

namespace Seeing {

PreviewPanel::PreviewPanel(ProjectModel& model, QWidget* parent)
    : QWidget(parent)
    , m_model(model)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    // ── Preview area ────────────────────────────────────────────────────────
    m_previewLabel = new QLabel(this);
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setText("▶  PREVIEW");
    m_previewLabel->setStyleSheet(
        "QLabel {"
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1,"
        "    stop:0 #1a1a2e, stop:0.5 #16213e, stop:1 #0f3460);"
        "  color: #4a6fa5; font-size: 28px; font-weight: 300;"
        "  letter-spacing: 6px; border: none;"
        "}");
    layout->addWidget(m_previewLabel, 1);

    // ── Info bar ────────────────────────────────────────────────────────────
    m_infoLabel = new QLabel("No clips on timeline", this);
    m_infoLabel->setAlignment(Qt::AlignCenter);
    m_infoLabel->setStyleSheet(
        "background: #252526; color: #888888; font-size: 11px;"
        "padding: 4px; border-top: 1px solid #3c3c3c;");
    layout->addWidget(m_infoLabel);

    // ── React to model changes ──────────────────────────────────────────────
    connect(&m_model, &ProjectModel::modelChanged,
            this, &PreviewPanel::onModelChanged);
}

void PreviewPanel::onModelChanged()
{
    auto clips = m_model.clips();
    if (clips.isEmpty()) {
        m_infoLabel->setText("No clips on timeline");
    } else {
        double totalDuration = 0;
        for (const auto& c : clips) {
            double end = c.timelineEnd;
            if (end > totalDuration) totalDuration = end;
        }
        m_infoLabel->setText(
            QString("%1 clip(s) | Duration: %2s | Tracks: %3")
                .arg(clips.size())
                .arg(totalDuration, 0, 'f', 1)
                .arg(m_model.trackCount()));
    }
}

} // namespace Seeing
