///////////////////////////////////////////////////////////////////////////////
/// @file   chat_panel.cpp
/// @brief  AI Copilot chat panel — sends prompts to the Controller.
///
/// This panel is purely a View component. It:
///   1. Captures user text input.
///   2. Forwards it to the Controller (never directly to the AI).
///   3. Displays responses from the Controller/AI.
///////////////////////////////////////////////////////////////////////////////

#include "chat_panel.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFont>
#include <QDateTime>
#include <QScrollBar>

namespace Seeing {

ChatPanel::ChatPanel(EditorController& controller, QWidget* parent)
    : QWidget(parent)
    , m_controller(controller)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    // ── Title ───────────────────────────────────────────────────────────────
    m_titleLabel = new QLabel("AI COPILOT", this);
    m_titleLabel->setStyleSheet(
        "color: #888888; font-size: 11px; font-weight: bold;"
        "letter-spacing: 1px; padding-bottom: 4px;");
    layout->addWidget(m_titleLabel);

    // ── Engine indicator ────────────────────────────────────────────────────
    auto* engineLabel = new QLabel(
        QString("🤖  %1").arg(m_controller.currentEngineName()), this);
    engineLabel->setStyleSheet(
        "color: #569cd6; font-size: 11px; padding: 4px 8px;"
        "background: #1e2d3d; border-radius: 4px;");
    layout->addWidget(engineLabel);

    connect(&m_controller, &EditorController::engineChanged,
            this, [engineLabel](const QString& name) {
                engineLabel->setText(QString("🤖  %1").arg(name));
            });

    // ── Chat History ────────────────────────────────────────────────────────
    m_chatHistory = new QTextEdit(this);
    m_chatHistory->setReadOnly(true);
    m_chatHistory->setStyleSheet(
        "QTextEdit {"
        "  background: #1e1e1e; border: 1px solid #3c3c3c;"
        "  border-radius: 4px; color: #cccccc; font-size: 12px;"
        "  padding: 8px; font-family: 'Inter', 'SF Pro Display', 'Segoe UI', 'Noto Sans', 'Menlo', 'Consolas', 'DejaVu Sans Mono', monospace;"
        "}");
    layout->addWidget(m_chatHistory, 1);

    // Welcome message
    m_chatHistory->setHtml(
        "<div style='color:#569cd6; margin-bottom:8px;'>"
        "<b>🎬 Seeing AI Copilot</b></div>"
        "<div style='color:#888; margin-bottom:12px;'>"
        "Type commands to edit your timeline.<br>"
        "Try: <span style='color:#ce9178;'>add demo</span> or "
        "<span style='color:#ce9178;'>help</span></div>"
        "<hr style='border-color:#3c3c3c;'>");

    // ── Input area ──────────────────────────────────────────────────────────
    auto* inputLayout = new QHBoxLayout();
    inputLayout->setSpacing(4);

    m_inputField = new QLineEdit(this);
    m_inputField->setPlaceholderText("Type a command...");
    m_inputField->setStyleSheet(
        "QLineEdit {"
        "  background: #3c3c3c; border: 1px solid #555555;"
        "  border-radius: 4px; color: #cccccc; padding: 8px;"
        "  font-size: 12px; font-family: 'Inter', 'SF Pro Display', 'Segoe UI', 'Noto Sans', sans-serif;"
        "}"
        "QLineEdit:focus {"
        "  border: 1px solid #007acc;"
        "}");
    inputLayout->addWidget(m_inputField, 1);

    m_sendBtn = new QPushButton("▶", this);
    m_sendBtn->setFixedSize(36, 36);
    m_sendBtn->setStyleSheet(
        "QPushButton {"
        "  background: #0e639c; color: white; border: none;"
        "  border-radius: 4px; font-size: 14px; font-weight: bold;"
        "}"
        "QPushButton:hover { background: #1177bb; }"
        "QPushButton:pressed { background: #094771; }");
    inputLayout->addWidget(m_sendBtn);

    layout->addLayout(inputLayout);

    // ── Connections ─────────────────────────────────────────────────────────
    connect(m_sendBtn,    &QPushButton::clicked,
            this,         &ChatPanel::onSendClicked);
    connect(m_inputField, &QLineEdit::returnPressed,
            this,         &ChatPanel::onSendClicked);
    connect(&m_controller, &EditorController::chatResponse,
            this,          &ChatPanel::onChatResponse);

    setStyleSheet("background: #1e1e1e;");
}

void ChatPanel::onSendClicked()
{
    QString text = m_inputField->text().trimmed();
    if (text.isEmpty()) return;

    m_inputField->clear();
    m_inputField->setFocus();

    // Route to Controller
    m_controller.handleChatPrompt(text);
}

void ChatPanel::onChatResponse(const QString& sender, const QString& message)
{
    QString time = QDateTime::currentDateTime().toString("HH:mm");
    QString color;
    QString icon;

    if (sender == "You") {
        color = "#dcdcaa";
        icon  = "👤";
    } else if (sender == "AI") {
        color = "#569cd6";
        icon  = "🤖";
    } else {
        color = "#6a9955";
        icon  = "⚙️";
    }

    // Format message (preserve newlines)
    QString htmlMsg = message.toHtmlEscaped().replace("\n", "<br>");

    QString html = QString(
        "<div style='margin: 4px 0;'>"
        "<span style='color:%1; font-weight:bold;'>%2 %3</span>"
        " <span style='color:#555; font-size:10px;'>%4</span>"
        "<br>"
        "<span style='color:#ccc; margin-left:8px;'>%5</span>"
        "</div>")
        .arg(color, icon, sender, time, htmlMsg);

    m_chatHistory->append(html);

    // Auto-scroll to bottom
    QScrollBar* sb = m_chatHistory->verticalScrollBar();
    sb->setValue(sb->maximum());
}

} // namespace Seeing
