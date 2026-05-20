///////////////////////////////////////////////////////////////////////////////
/// @file   settings_dialog.h
/// @brief  Settings Dialog for configuring AI engines (Cloud & Local).
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QStackedWidget>
#include <QPushButton>
#include <QLabel>

#include "controller/editor_controller.h"

namespace Seeing {

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(EditorController& controller, QWidget* parent = nullptr);
    ~SettingsDialog() override = default;

private slots:
    void onEngineChanged(int index);
    void onSave();
    void onTestConnection();

private:
    void setupUI();
    void loadSettings();
    void applyTheme();

    EditorController& m_controller;

    // Common Controls
    QComboBox* m_vlmCombo = nullptr;
    QComboBox* m_engineCombo = nullptr;
    QStackedWidget* m_stackedWidget = nullptr;

    // OpenAI Panel Controls
    QLineEdit* m_openaiKey = nullptr;
    QLineEdit* m_openaiModel = nullptr;

    // Gemini Panel Controls
    QLineEdit* m_geminiKey = nullptr;
    QLineEdit* m_geminiModel = nullptr;

    // Ollama Panel Controls
    QLineEdit* m_ollamaUrl = nullptr;
    QLineEdit* m_ollamaModel = nullptr;

    // Marlin Panel Controls
    QLineEdit* m_hfToken = nullptr;

    // Test & Action buttons
    QLabel* m_vlmStatusLabel = nullptr;
    QPushButton* m_testBtn = nullptr;
    QLabel* m_testStatus = nullptr;
    QPushButton* m_saveBtn = nullptr;
    QPushButton* m_cancelBtn = nullptr;

    void checkVlmStatus();
};

} // namespace Seeing
