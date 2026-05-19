///////////////////////////////////////////////////////////////////////////////
/// @file   dummy_ai_engine.h
/// @brief  A mock AI engine for MVP testing.
///
/// Parses simple text commands (no real AI) and returns JSON mutations.
/// This allows full end-to-end testing of the MVC pipeline.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "ai_engine_interface.h"

namespace Seeing {

class DummyAiEngine : public AiEngineInterface
{
    Q_OBJECT

public:
    explicit DummyAiEngine(QObject* parent = nullptr);

    QString     engineName()  const override;
    bool        isCloudBased() const override;
    QJsonObject processPrompt(const QString& prompt,
                              const QJsonObject& projectState) override;

private:
    /// Simple keyword-based command parser.
    QJsonObject parseAddClip(const QString& prompt);
    QJsonObject parseRemoveClip(const QString& prompt);
    QJsonObject parseTrimClip(const QString& prompt);
    QJsonObject parseMoveClip(const QString& prompt);
};

} // namespace Seeing
