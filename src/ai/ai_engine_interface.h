///////////////////////////////////////////////////////////////////////////////
/// @file   ai_engine_interface.h
/// @brief  Abstract interface for all AI engines (Local & Cloud).
///
/// Every AI backend — whether OpenAI, Gemini, Claude, Llama.cpp, or Ollama —
/// must implement this interface. The Controller only talks to this interface,
/// making the AI layer fully pluggable.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>

namespace Seeing {

///////////////////////////////////////////////////////////////////////////////
/// @class AiEngineInterface
/// @brief Pure-virtual interface that all AI engines must implement.
///
/// The processPrompt() method receives a user prompt plus the current project
/// state (as JSON). It must return a JSON "mutation command" that the
/// Controller will apply to the Model.
///
/// Example mutation command:
/// {
///   "action": "add_clip",
///   "params": {
///     "name": "Intro",
///     "track_index": 0,
///     "timeline_start": 0.0,
///     "timeline_end": 5.0
///   }
/// }
///////////////////////////////////////////////////////////////////////////////
class AiEngineInterface : public QObject
{
    Q_OBJECT

public:
    explicit AiEngineInterface(QObject* parent = nullptr)
        : QObject(parent) {}

    virtual ~AiEngineInterface() = default;

    /// Human-readable name of this engine (e.g. "OpenAI GPT-4", "Local Llama").
    virtual QString engineName() const = 0;

    /// Whether this engine requires network access.
    virtual bool isCloudBased() const = 0;

    /// Process a user prompt and return a JSON mutation command.
    /// @param prompt           The raw user text from the chat panel.
    /// @param projectState     The current Master JSON state (read-only context).
    /// @return                 A JSON object describing the mutation to apply.
    virtual QJsonObject processPrompt(const QString& prompt,
                                      const QJsonObject& projectState) = 0;

signals:
    /// Emitted when the engine has a status message (for chat display).
    void statusMessage(const QString& message);
};

} // namespace Seeing
