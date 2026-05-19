///////////////////////////////////////////////////////////////////////////////
/// @file   editor_controller.h
/// @brief  The CONTROLLER layer — routes user actions to the Model.
///
/// The Controller sits between the View and Model. It:
///   1. Receives user input (chat prompts, UI interactions).
///   2. Routes prompts to the active AI Engine.
///   3. Interprets the AI's JSON mutation command.
///   4. Applies the mutation to the ProjectModel.
///
/// The View never directly mutates the Model — it always goes through here.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <QObject>
#include <memory>

#include "model/project_model.h"
#include "ai/ai_engine_interface.h"
#include "ai/ai_engine_factory.h"

namespace Seeing {

class EditorController : public QObject
{
    Q_OBJECT

public:
    explicit EditorController(ProjectModel& model, QObject* parent = nullptr);

    /// Process a text prompt from the chat panel.
    /// Routes to the AI engine, gets a mutation command, and applies it.
    void handleChatPrompt(const QString& prompt);

    /// Switch the active AI engine at runtime.
    void setAiEngine(AiEngineFactory::EngineType type);
    void setAiEngine(const QString& engineName);

    /// Get the name of the currently active AI engine.
    QString currentEngineName() const;

    /// Direct model manipulation (for UI drag/drop, manual edits).
    void addAssetFromFile(const QString& filePath);
    void removeAsset(const QString& assetId);

    /// Undo / Redo passthrough.
    void undo();
    void redo();

signals:
    /// Emitted with messages for the chat display.
    void chatResponse(const QString& sender, const QString& message);

    /// Emitted when the AI engine changes.
    void engineChanged(const QString& engineName);

private:
    /// Apply a JSON mutation command to the model.
    void applyMutationCommand(const QJsonObject& command);

    ProjectModel&                       m_model;
    std::unique_ptr<AiEngineInterface>  m_aiEngine;
};

} // namespace Seeing
