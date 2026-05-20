///////////////////////////////////////////////////////////////////////////////
/// @file   ai_engine_factory.h
/// @brief  Factory for creating AI engine instances by name.
///
/// Centralizes AI engine construction so the Controller never directly
/// instantiates a concrete engine class. Adding a new engine (e.g. Ollama,
/// OpenAI) only requires modifying this factory.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "ai_engine_interface.h"
#include <QString>
#include <memory>

namespace Seeing {

class AiEngineFactory
{
public:
    /// Supported engine identifiers.
    enum class EngineType {
        Dummy,
        OpenAI,
        Gemini,
        Ollama
    };

    /// Create an engine instance.
    /// @param type    Which engine to instantiate.
    /// @param parent  QObject parent for memory management.
    /// @return        Unique pointer to the new engine.
    static std::unique_ptr<AiEngineInterface> create(EngineType type,
                                                      QObject* parent = nullptr);

    /// Create an engine by string name (for config files / UI dropdowns).
    static std::unique_ptr<AiEngineInterface> create(const QString& name,
                                                      QObject* parent = nullptr);
};

} // namespace Seeing
