///////////////////////////////////////////////////////////////////////////////
/// @file   media_panel.cpp
/// @brief  Media Pool panel — import assets and display them in a list.
///////////////////////////////////////////////////////////////////////////////

#include "media_panel.h"

#include <QVBoxLayout>
#include <QFileDialog>
#include <QFont>

namespace Seeing {

MediaPanel::MediaPanel(ProjectModel& model,
                       EditorController& controller,
                       QWidget* parent)
    : QWidget(parent)
    , m_model(model)
    , m_controller(controller)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    // ── Title ───────────────────────────────────────────────────────────────
    m_titleLabel = new QLabel("EXPLORER", this);
    m_titleLabel->setStyleSheet(
        "color: #888888; font-size: 11px; font-weight: bold; "
        "letter-spacing: 1px; padding-bottom: 4px;");
    layout->addWidget(m_titleLabel);

    // ── Media Pool subtitle ─────────────────────────────────────────────────
    auto* subtitle = new QLabel("📂  Media Pool", this);
    subtitle->setStyleSheet("color: #cccccc; font-size: 13px; font-weight: 600;");
    layout->addWidget(subtitle);

    // ── Asset List ──────────────────────────────────────────────────────────
    m_assetList = new QListWidget(this);
    m_assetList->setStyleSheet(
        "QListWidget {"
        "  background: #252526; border: 1px solid #3c3c3c; border-radius: 4px;"
        "  color: #cccccc; font-size: 12px;"
        "}"
        "QListWidget::item {"
        "  padding: 6px 8px; border-bottom: 1px solid #2d2d2d;"
        "}"
        "QListWidget::item:selected {"
        "  background: #094771; color: white;"
        "}"
        "QListWidget::item:hover {"
        "  background: #2a2d2e;"
        "}");
    m_assetList->setAlternatingRowColors(false);
    layout->addWidget(m_assetList, 1);

    // ── Import Button ───────────────────────────────────────────────────────
    m_importBtn = new QPushButton("+ Import Media", this);
    m_importBtn->setStyleSheet(
        "QPushButton {"
        "  background: #0e639c; color: white; border: none;"
        "  border-radius: 4px; padding: 8px; font-weight: bold;"
        "  font-size: 12px;"
        "}"
        "QPushButton:hover { background: #1177bb; }"
        "QPushButton:pressed { background: #094771; }");
    layout->addWidget(m_importBtn);

    // ── Connections ─────────────────────────────────────────────────────────
    connect(m_importBtn, &QPushButton::clicked, this, &MediaPanel::onImportClicked);
    connect(&m_model, &ProjectModel::modelChanged, this, &MediaPanel::refreshAssetList);

    setStyleSheet("background: #1e1e1e;");
}

void MediaPanel::onImportClicked()
{
    QString filePath = QFileDialog::getOpenFileName(
        this, "Import Media",
        QString(),
        "Media Files (*.mp4 *.mov *.avi *.mkv *.mp3 *.wav *.png *.jpg);;All Files (*)");

    if (!filePath.isEmpty()) {
        m_controller.addAssetFromFile(filePath);
    }
}

void MediaPanel::refreshAssetList()
{
    m_assetList->clear();
    const auto assets = m_model.assets();
    for (const auto& asset : assets) {
        QString icon;
        if (asset.type == "video")      icon = "🎬";
        else if (asset.type == "audio") icon = "🎵";
        else if (asset.type == "image") icon = "🖼️";
        else                            icon = "📄";

        QString text = QString("%1  %2").arg(icon, asset.name);
        if (asset.type != "image") {
            text += QString("  (%1s)").arg(asset.duration, 0, 'f', 1);
        }

        auto* item = new QListWidgetItem(text, m_assetList);

        // Tooltip displaying path and AI description
        QString tooltip = QString("📄 Name: %1\n📁 Path: %2\n🏷️ Type: %3")
            .arg(asset.name, asset.filePath, asset.type);

        if (!asset.description.isEmpty()) {
            tooltip += QString("\n\n🤖 AI Copilot Analysis:\n%1").arg(asset.description);
        }
        item->setToolTip(tooltip);
    }
}

} // namespace Seeing
