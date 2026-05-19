///////////////////////////////////////////////////////////////////////////////
/// @file   media_panel.h
/// @brief  Left panel — Media Pool / Asset Explorer.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>

#include "model/project_model.h"
#include "controller/editor_controller.h"

namespace Seeing {

class MediaPanel : public QWidget
{
    Q_OBJECT

public:
    explicit MediaPanel(ProjectModel& model,
                        EditorController& controller,
                        QWidget* parent = nullptr);

private slots:
    void onImportClicked();
    void refreshAssetList();

private:
    ProjectModel&     m_model;
    EditorController& m_controller;

    QLabel*       m_titleLabel  = nullptr;
    QListWidget*  m_assetList   = nullptr;
    QPushButton*  m_importBtn   = nullptr;
};

} // namespace Seeing
