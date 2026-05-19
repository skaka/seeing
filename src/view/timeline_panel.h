///////////////////////////////////////////////////////////////////////////////
/// @file   timeline_panel.h
/// @brief  Center-bottom panel — QGraphicsScene-based multi-track timeline.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <QWidget>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QLabel>

#include "model/project_model.h"

namespace Seeing {

class TimelinePanel : public QWidget
{
    Q_OBJECT

public:
    explicit TimelinePanel(ProjectModel& model, QWidget* parent = nullptr);

private slots:
    /// Rebuild the entire scene from the Model.
    void rebuildTimeline();

private:
    void drawTrackBackgrounds();
    void drawTimeRuler();
    void drawClips();
    void drawPlayhead();

    ProjectModel&   m_model;
    QGraphicsScene* m_scene       = nullptr;
    QGraphicsView*  m_view        = nullptr;
    QLabel*         m_headerLabel = nullptr;

    // Layout constants
    static constexpr double kPixelsPerSecond = 60.0;
    static constexpr double kTrackHeight     = 60.0;
    static constexpr double kHeaderHeight    = 26.0;
    static constexpr double kRulerHeight     = 22.0;
    static constexpr double kSceneWidth      = 3600.0; // 60 seconds visible
};

} // namespace Seeing
