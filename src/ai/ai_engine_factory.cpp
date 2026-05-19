///////////////////////////////////////////////////////////////////////////////
/// @file   ai_engine_factory.cpp
/// @brief  Factory implementation — currently only creates DummyAiEngine.
///////////////////////////////////////////////////////////////////////////////

#include "ai_engine_factory.h"
#include "dummy_ai_engine.h"

namespace Seeing {

std::unique_ptr<AiEngineInterface>
AiEngineFactory::create(EngineType type, QObject* parent)
{
    switch (type) {
    case EngineType::Dummy:
        return std::make_unique<DummyAiEngine>(parent);

    // Future engines:
    // case EngineType::OpenAI:
    //     return std::make_unique<OpenAiEngine>(parent);
    // case EngineType::Ollama:
    //     return std::make_unique<OllamaEngine>(parent);

    default:
        return std::make_unique<DummyAiEngine>(parent);
    }
}

std::unique_ptr<AiEngineInterface>
AiEngineFactory::create(const QString& name, QObject* parent)
{
    QString lower = name.toLower().trimmed();

    if (lower == "dummy" || lower == "mock" || lower == "mvp")
        return create(EngineType::Dummy, parent);

    // Future:
    // if (lower == "openai" || lower == "gpt")
    //     return create(EngineType::OpenAI, parent);

    // Default fallback
    return create(EngineType::Dummy, parent);
}

} // namespace Seeing
