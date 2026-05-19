///////////////////////////////////////////////////////////////////////////////
/// @file   preview_panel.h
/// @brief  Center-top panel — Video preview (placeholder for MVP).
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <QWidget>
#include <QLabel>
#include "model/project_model.h"

namespace Seeing {

class PreviewPanel : public QWidget
{
    Q_OBJECT

public:
    explicit PreviewPanel(ProjectModel& model, QWidget* parent = nullptr);

private slots:
    void onModelChanged();

private:
    ProjectModel& m_model;
    QLabel*        m_previewLabel  = nullptr;
    QLabel*        m_infoLabel     = nullptr;
};

} // namespace Seeing
