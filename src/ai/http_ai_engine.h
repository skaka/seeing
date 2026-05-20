///////////////////////////////////////////////////////////////////////////////
/// @file   http_ai_engine.h
/// @brief  Generic HTTP AI Engine for cloud APIs (OpenAI, Gemini) and local models (Ollama).
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "ai_engine_interface.h"
#include <QNetworkAccessManager>

namespace Seeing {

class HttpAiEngine : public AiEngineInterface
{
    Q_OBJECT

public:
    enum class Mode {
        OpenAI,
        Gemini,
        Ollama
    };

    explicit HttpAiEngine(Mode mode, QObject* parent = nullptr);
    ~HttpAiEngine() override = default;

    QString     engineName()   const override;
    bool        isCloudBased() const override;
    QJsonObject processPrompt(const QString& prompt,
                              const QJsonObject& projectState) override;

private:
    QJsonObject queryOpenAI(const QString& prompt, const QString& systemPrompt);
    QJsonObject queryGemini(const QString& prompt, const QString& systemPrompt);
    QJsonObject queryOllama(const QString& prompt, const QString& systemPrompt);

    QJsonObject executeRequest(const QUrl& url, const QJsonObject& body, const QMap<QString, QString>& headers);
    QJsonObject cleanAndParseResponse(const QString& responseText);

    Mode m_mode;
    QNetworkAccessManager m_networkManager;
};

} // namespace Seeing
