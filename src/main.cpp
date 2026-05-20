///////////////////////////////////////////////////////////////////////////////
/// @file   main.cpp
/// @brief  Entry point for the Seeing NLE application.
///
/// Bootstraps the Qt application, wires Model → Controller → View,
/// and displays the main window.
///////////////////////////////////////////////////////////////////////////////

#include <QApplication>
#include <QFontDatabase>
#include <QStyleFactory>
#include <QSettings>

#include "model/project_model.h"
#include "controller/editor_controller.h"
#include "view/main_window.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Seeing");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("SeeingTeam");
    QSettings::setDefaultFormat(QSettings::IniFormat);

    // ── Dark Fusion palette (VS Code-inspired) ─────────────────────────────
    app.setStyle(QStyleFactory::create("Fusion"));

    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window,          QColor(30,  30,  30));
    darkPalette.setColor(QPalette::WindowText,      QColor(212, 212, 212));
    darkPalette.setColor(QPalette::Base,            QColor(37,  37,  38));
    darkPalette.setColor(QPalette::AlternateBase,   QColor(45,  45,  48));
    darkPalette.setColor(QPalette::ToolTipBase,     QColor(37,  37,  38));
    darkPalette.setColor(QPalette::ToolTipText,     QColor(212, 212, 212));
    darkPalette.setColor(QPalette::Text,            QColor(212, 212, 212));
    darkPalette.setColor(QPalette::Button,          QColor(51,  51,  51));
    darkPalette.setColor(QPalette::ButtonText,      QColor(212, 212, 212));
    darkPalette.setColor(QPalette::BrightText,      QColor(255, 255, 255));
    darkPalette.setColor(QPalette::Link,            QColor(86,  156, 214));
    darkPalette.setColor(QPalette::Highlight,       QColor(38,  79,  120));
    darkPalette.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
    darkPalette.setColor(QPalette::Disabled, QPalette::Text,       QColor(128, 128, 128));
    darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(128, 128, 128));

    app.setPalette(darkPalette);

    // Global stylesheet for fine-tuning
    app.setStyleSheet(R"(
        QToolTip {
            color: #d4d4d4;
            background-color: #252526;
            border: 1px solid #3c3c3c;
            padding: 4px;
        }
        QSplitter::handle {
            background-color: #3c3c3c;
        }
        QScrollBar:vertical {
            background: #1e1e1e;
            width: 10px;
        }
        QScrollBar::handle:vertical {
            background: #424242;
            min-height: 20px;
            border-radius: 4px;
        }
        QScrollBar:horizontal {
            background: #1e1e1e;
            height: 10px;
        }
        QScrollBar::handle:horizontal {
            background: #424242;
            min-width: 20px;
            border-radius: 4px;
        }
        QScrollBar::add-line, QScrollBar::sub-line {
            height: 0px;
            width: 0px;
        }
    )");

    // ── MVC Wiring ──────────────────────────────────────────────────────────
    // 1. Create the Model (single source of truth)
    Seeing::ProjectModel model;

    // 2. Create the Controller (mutates the Model)
    Seeing::EditorController controller(model);

    // 3. Create the View (observes the Model)
    Seeing::MainWindow mainWindow(model, controller);
    mainWindow.setWindowTitle("Seeing — AI Video Editor");
    mainWindow.resize(1400, 850);
    mainWindow.show();

    return app.exec();
}
