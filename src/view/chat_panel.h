///////////////////////////////////////////////////////////////////////////////
/// @file   chat_panel.h
/// @brief  Right panel — AI Copilot chat interface.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <QWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

#include "controller/editor_controller.h"

namespace Seeing {

class ChatPanel : public QWidget
{
    Q_OBJECT

public:
    explicit ChatPanel(EditorController& controller,
                       QWidget* parent = nullptr);

private slots:
    void onSendClicked();
    void onChatResponse(const QString& sender, const QString& message);

private:
    EditorController& m_controller;

    QLabel*      m_titleLabel    = nullptr;
    QTextEdit*   m_chatHistory   = nullptr;
    QLineEdit*   m_inputField    = nullptr;
    QPushButton* m_sendBtn       = nullptr;
};

} // namespace Seeing
