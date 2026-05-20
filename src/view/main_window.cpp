///////////////////////////////////////////////////////////////////////////////
/// @file   main_window.cpp
/// @brief  Main window — assembles the 4-panel layout.
///
/// Layout structure:
///   ┌───────────┬────────────────────────┬───────────┐
///   │           │     Preview Panel      │           │
///   │   Media   ├────────────────────────┤    AI     │
///   │   Panel   │     Timeline Panel     │   Chat    │
///   │           │   (QGraphicsView)      │   Panel   │
///   └───────────┴────────────────────────┴───────────┘
///////////////////////////////////////////////////////////////////////////////

#include "main_window.h"

#include "view/media_panel.h"
#include "view/preview_panel.h"
#include "view/timeline_panel.h"
#include "view/chat_panel.h"
#include "view/settings_dialog.h"

#include <QMenuBar>
#include <QStatusBar>
#include <QLabel>
#include <QAction>
#include <QShortcut>

namespace Seeing {

MainWindow::MainWindow(ProjectModel& model,
                       EditorController& controller,
                       QWidget* parent)
    : QMainWindow(parent)
    , m_model(model)
    , m_controller(controller)
{
    setupLayout();
    setupMenuBar();
    setupStatusBar();
    connectSignals();
}

void MainWindow::setupLayout()
{
    // ── Create panels ───────────────────────────────────────────────────────
    m_mediaPanel    = new MediaPanel(m_model, m_controller, this);
    m_previewPanel  = new PreviewPanel(m_model, this);
    m_timelinePanel = new TimelinePanel(m_model, this);
    m_chatPanel     = new ChatPanel(m_controller, this);

    // ── Center vertical split: Preview (top) | Timeline (bottom) ────────
    m_centerSplitter = new QSplitter(Qt::Vertical, this);
    m_centerSplitter->addWidget(m_previewPanel);
    m_centerSplitter->addWidget(m_timelinePanel);
    m_centerSplitter->setStretchFactor(0, 2);   // Preview gets 2/5
    m_centerSplitter->setStretchFactor(1, 3);   // Timeline gets 3/5
    m_centerSplitter->setHandleWidth(2);

    // ── Main horizontal split: Media | Center | Chat ────────────────────
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    m_mainSplitter->addWidget(m_mediaPanel);
    m_mainSplitter->addWidget(m_centerSplitter);
    m_mainSplitter->addWidget(m_chatPanel);
    m_mainSplitter->setStretchFactor(0, 1);     // Media = narrow
    m_mainSplitter->setStretchFactor(1, 4);     // Center = wide
    m_mainSplitter->setStretchFactor(2, 1);     // Chat = narrow
    m_mainSplitter->setHandleWidth(2);

    // Set minimum sizes
    m_mediaPanel->setMinimumWidth(180);
    m_chatPanel->setMinimumWidth(220);
    m_previewPanel->setMinimumHeight(150);
    m_timelinePanel->setMinimumHeight(200);

    setCentralWidget(m_mainSplitter);
}

void MainWindow::setupMenuBar()
{
    auto* menuBar = this->menuBar();
    menuBar->setStyleSheet(
        "QMenuBar { background: #252526; color: #cccccc; border-bottom: 1px solid #3c3c3c; }"
        "QMenuBar::item:selected { background: #094771; }"
        "QMenu { background: #252526; color: #cccccc; border: 1px solid #3c3c3c; }"
        "QMenu::item:selected { background: #094771; }");

    // ── File menu ───────────────────────────────────────────────────────────
    auto* fileMenu = menuBar->addMenu("&File");
    fileMenu->addAction("&New Project",   this, []() {}, QKeySequence::New);
    fileMenu->addAction("&Open Project",  this, []() {}, QKeySequence::Open);
    fileMenu->addAction("&Save Project",  this, []() {}, QKeySequence::Save);
    fileMenu->addSeparator();
    fileMenu->addAction("E&xit",          this, &QWidget::close, QKeySequence::Quit);

    // ── Edit menu ───────────────────────────────────────────────────────────
    auto* editMenu = menuBar->addMenu("&Edit");
    editMenu->addAction("&Undo", this,
        [this]() { m_controller.undo(); }, QKeySequence::Undo);
    editMenu->addAction("&Redo", this,
        [this]() { m_controller.redo(); }, QKeySequence::Redo);

    // ── View menu ───────────────────────────────────────────────────────────
    auto* viewMenu = menuBar->addMenu("&View");
    viewMenu->addAction("Toggle &Media Panel",  m_mediaPanel,   &QWidget::setVisible);
    viewMenu->addAction("Toggle &Chat Panel",   m_chatPanel,    &QWidget::setVisible);

    // ── AI menu ─────────────────────────────────────────────────────────────
    auto* aiMenu = menuBar->addMenu("&AI");
    aiMenu->addAction("Configure &AI Copilot...", this, [this]() {
        SettingsDialog dlg(m_controller, this);
        dlg.exec();
    }, QKeySequence(Qt::CTRL | Qt::Key_Comma));
}

void MainWindow::setupStatusBar()
{
    auto* status = statusBar();
    status->setStyleSheet(
        "QStatusBar { background: #007acc; color: white; font-size: 12px; }"
        "QStatusBar::item { border: none; }");

    auto* engineLabel = new QLabel(
        QString("  AI: %1  ").arg(m_controller.currentEngineName()), this);
    engineLabel->setStyleSheet("color: white; font-weight: bold;");
    status->addPermanentWidget(engineLabel);

    status->showMessage("Ready");

    // Update engine label when engine changes
    connect(&m_controller, &EditorController::engineChanged,
            this, [engineLabel](const QString& name) {
                engineLabel->setText(QString("  AI: %1  ").arg(name));
            });
}

void MainWindow::connectSignals()
{
    // Keyboard shortcuts
    auto* undoShortcut = new QShortcut(QKeySequence::Undo, this);
    connect(undoShortcut, &QShortcut::activated, this,
            [this]() { m_controller.undo(); });

    auto* redoShortcut = new QShortcut(QKeySequence::Redo, this);
    connect(redoShortcut, &QShortcut::activated, this,
            [this]() { m_controller.redo(); });
}

} // namespace Seeing
