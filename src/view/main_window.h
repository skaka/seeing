///////////////////////////////////////////////////////////////////////////////
/// @file   main_window.h
/// @brief  Top-level window with VS Code-style 4-panel layout.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <QMainWindow>
#include <QSplitter>

#include "model/project_model.h"
#include "controller/editor_controller.h"

// Forward declarations
namespace Seeing {
class MediaPanel;
class PreviewPanel;
class TimelinePanel;
class ChatPanel;
}

namespace Seeing {

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(ProjectModel& model,
                        EditorController& controller,
                        QWidget* parent = nullptr);
    ~MainWindow() override = default;

private:
    void setupLayout();
    void setupMenuBar();
    void setupStatusBar();
    void connectSignals();

    // MVC references
    ProjectModel&     m_model;
    EditorController& m_controller;

    // Panels
    MediaPanel*    m_mediaPanel    = nullptr;
    PreviewPanel*  m_previewPanel  = nullptr;
    TimelinePanel* m_timelinePanel = nullptr;
    ChatPanel*     m_chatPanel     = nullptr;

    // Layout
    QSplitter* m_mainSplitter     = nullptr;
    QSplitter* m_centerSplitter   = nullptr;
};

} // namespace Seeing
