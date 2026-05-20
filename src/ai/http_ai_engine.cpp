#include "http_ai_engine.h"

#include <QSettings>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>

namespace Seeing {

HttpAiEngine::HttpAiEngine(Mode mode, QObject* parent)
    : AiEngineInterface(parent)
    , m_mode(mode)
{
}

QString HttpAiEngine::engineName() const
{
    QSettings settings;
    switch (m_mode) {
    case Mode::OpenAI: {
        QString model = settings.value("ai/openai_model", "gpt-4o").toString();
        return QString("OpenAI (%1)").arg(model);
    }
    case Mode::Gemini: {
        QString model = settings.value("ai/gemini_model", "gemini-2.5-flash").toString();
        return QString("Gemini (%1)").arg(model);
    }
    case Mode::Ollama: {
        QString model = settings.value("ai/ollama_model", "llama3").toString();
        return QString("Ollama (%1)").arg(model);
    }
    }
    return "HTTP AI Engine";
}

bool HttpAiEngine::isCloudBased() const
{
    return (m_mode == Mode::OpenAI || m_mode == Mode::Gemini);
}

QJsonObject HttpAiEngine::processPrompt(const QString& prompt,
                                        const QJsonObject& projectState)
{
    emit statusMessage("🤖 AI is thinking...");

    // ── Build system instructions / schema ──────────────────────────────────
    QString stateStr = QJsonDocument(projectState).toJson(QJsonDocument::Compact);
    QString systemPrompt =
        "You are the AI copilot for 'Seeing', a non-linear video editor.\n"
        "Your task is to translate user text prompts into timeline mutation commands in JSON format.\n\n"
        "Current Timeline State:\n"
        + stateStr + "\n\n"
        "Supported actions:\n"
        "1. Add a clip:\n"
        "   {\"action\": \"add_clip\", \"params\": {\"name\": \"ClipName\", \"track_index\": 0, \"timeline_start\": 0.0, \"timeline_end\": 5.0, \"color\": \"#hex\", \"asset_id\": \"assetIdFromState\"}}\n"
        "2. Remove a clip:\n"
        "   {\"action\": \"remove_clip\", \"params\": {\"name\": \"ClipName\"}}\n"
        "3. Trim a clip:\n"
        "   {\"action\": \"trim_clip\", \"params\": {\"name\": \"ClipName\", \"duration\": 10.0}}\n"
        "4. Move a clip:\n"
        "   {\"action\": \"move_clip\", \"params\": {\"name\": \"ClipName\", \"position\": 15.0}}\n"
        "5. Execute multiple actions sequentially (e.g., for automatic editing/montage of assets):\n"
        "   {\"action\": \"sequence\", \"params\": {\"commands\": [ {\"action\": \"add_clip\", ...}, {\"action\": \"move_clip\", ...} ]}}\n"
        "6. Undo last action:\n"
        "   {\"action\": \"undo\"}\n"
        "7. Redo last action:\n"
        "   {\"action\": \"redo\"}\n"
        "8. Unrecognized command:\n"
        "   {\"action\": \"unknown\"}\n\n"
        "Crucial Rules:\n"
        "- Respond ONLY with a valid JSON object matching the schemas above.\n"
        "- Do NOT wrap the JSON in markdown code blocks like ```json ... ```.\n"
        "- Do NOT add explanations or other words. Just the raw JSON.\n"
        "- To perform automated NLE montages/editing of assets, use the 'sequence' action. Generate 'add_clip' actions sequentially for the assets present in the current project state.";

    QJsonObject mutation;

    switch (m_mode) {
    case Mode::OpenAI:
        mutation = queryOpenAI(prompt, systemPrompt);
        break;
    case Mode::Gemini:
        mutation = queryGemini(prompt, systemPrompt);
        break;
    case Mode::Ollama:
        mutation = queryOllama(prompt, systemPrompt);
        break;
    }

    return mutation;
}

QJsonObject HttpAiEngine::queryOpenAI(const QString& prompt, const QString& systemPrompt)
{
    QSettings settings;
    QString apiKey = settings.value("ai/openai_key", "").toString().trimmed();
    QString model  = settings.value("ai/openai_model", "gpt-4o").toString();

    if (apiKey.isEmpty()) {
        emit statusMessage("⚠️ OpenAI API key is missing. Set it in Settings.");
        QJsonObject err;
        err["action"] = "unknown";
        return err;
    }

    QUrl url("https://api.openai.com/v1/chat/completions");

    QJsonObject requestBody;
    requestBody["model"] = model;
    requestBody["temperature"] = 0.0;

    QJsonArray messages;
    QJsonObject sysMsg;
    sysMsg["role"] = "system";
    sysMsg["content"] = systemPrompt;
    messages.append(sysMsg);

    QJsonObject userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = prompt;
    messages.append(userMsg);

    requestBody["messages"] = messages;

    QMap<QString, QString> headers;
    headers["Content-Type"] = "application/json";
    headers["Authorization"] = "Bearer " + apiKey;

    QJsonObject response = executeRequest(url, requestBody, headers);
    if (response.isEmpty()) {
        QJsonObject err;
        err["action"] = "unknown";
        return err;
    }

    // Parse choices[0].message.content
    QJsonArray choices = response["choices"].toArray();
    if (choices.isEmpty()) {
        emit statusMessage("⚠️ Received empty or invalid response structure from OpenAI.");
        QJsonObject err;
        err["action"] = "unknown";
        return err;
    }

    QString textContent = choices[0].toObject()["message"].toObject()["content"].toString();
    return cleanAndParseResponse(textContent);
}

QJsonObject HttpAiEngine::queryGemini(const QString& prompt, const QString& systemPrompt)
{
    QSettings settings;
    QString apiKey = settings.value("ai/gemini_key", "").toString().trimmed();
    QString model  = settings.value("ai/gemini_model", "gemini-2.5-flash").toString();

    if (apiKey.isEmpty()) {
        emit statusMessage("⚠️ Gemini API key is missing. Set it in Settings.");
        QJsonObject err;
        err["action"] = "unknown";
        return err;
    }

    QUrl url(QString("https://generativelanguage.googleapis.com/v1beta/models/%1:generateContent?key=%2")
                 .arg(model, apiKey));

    QJsonObject requestBody;

    // Contents (User prompt)
    QJsonArray contents;
    QJsonObject contentObj;
    QJsonArray parts;
    QJsonObject partObj;
    partObj["text"] = prompt;
    parts.append(partObj);
    contentObj["parts"] = parts;
    contents.append(contentObj);
    requestBody["contents"] = contents;

    // System instruction
    QJsonObject systemInstruction;
    QJsonArray sysParts;
    QJsonObject sysPartObj;
    sysPartObj["text"] = systemPrompt;
    sysParts.append(sysPartObj);
    systemInstruction["parts"] = sysParts;
    requestBody["systemInstruction"] = systemInstruction;

    // Generation Config
    QJsonObject genConfig;
    genConfig["temperature"] = 0.0;
    requestBody["generationConfig"] = genConfig;

    QMap<QString, QString> headers;
    headers["Content-Type"] = "application/json";

    QJsonObject response = executeRequest(url, requestBody, headers);
    if (response.isEmpty()) {
        QJsonObject err;
        err["action"] = "unknown";
        return err;
    }

    // Parse candidates[0].content.parts[0].text
    QJsonArray candidates = response["candidates"].toArray();
    if (candidates.isEmpty()) {
        emit statusMessage("⚠️ Received empty or invalid response structure from Gemini.");
        QJsonObject err;
        err["action"] = "unknown";
        return err;
    }

    QJsonObject content = candidates[0].toObject()["content"].toObject();
    QJsonArray resParts = content["parts"].toArray();
    if (resParts.isEmpty()) {
        emit statusMessage("⚠️ No parts found in Gemini response candidate.");
        QJsonObject err;
        err["action"] = "unknown";
        return err;
    }

    QString textContent = resParts[0].toObject()["text"].toString();
    return cleanAndParseResponse(textContent);
}

QJsonObject HttpAiEngine::queryOllama(const QString& prompt, const QString& systemPrompt)
{
    QSettings settings;
    QString baseUrl = settings.value("ai/ollama_url", "http://localhost:11434").toString().trimmed();
    QString model   = settings.value("ai/ollama_model", "llama3").toString();

    if (baseUrl.endsWith("/")) {
        baseUrl.chop(1);
    }

    QUrl url(baseUrl + "/api/generate");

    QJsonObject requestBody;
    requestBody["model"] = model;
    requestBody["prompt"] = systemPrompt + "\n\nUser request: " + prompt;
    requestBody["stream"] = false;

    QJsonObject options;
    options["temperature"] = 0.0;
    requestBody["options"] = options;

    QMap<QString, QString> headers;
    headers["Content-Type"] = "application/json";

    QJsonObject response = executeRequest(url, requestBody, headers);
    if (response.isEmpty()) {
        QJsonObject err;
        err["action"] = "unknown";
        return err;
    }

    QString textContent = response["response"].toString();
    return cleanAndParseResponse(textContent);
}

QJsonObject HttpAiEngine::executeRequest(const QUrl& url, const QJsonObject& body, const QMap<QString, QString>& headers)
{
    QNetworkRequest request(url);
    
    // Add headers
    for (auto it = headers.begin(); it != headers.end(); ++it) {
        request.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
    }

    QByteArray bodyData = QJsonDocument(body).toJson();
    QNetworkReply* reply = m_networkManager.post(request, bodyData);

    // Block synchronously until finished using a local event loop
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        QString errorMsg = reply->errorString();
        emit statusMessage(QString("❌ HTTP Request failed: %1").arg(errorMsg));
        reply->deleteLater();
        return QJsonObject();
    }

    QByteArray responseData = reply->readAll();
    reply->deleteLater();

    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    if (!doc.isObject()) {
        emit statusMessage("❌ Failed to parse HTTP response as JSON.");
        return QJsonObject();
    }

    return doc.object();
}

QJsonObject HttpAiEngine::cleanAndParseResponse(const QString& responseText)
{
    QString clean = responseText.trimmed();

    // Remove markdown code blocks if the model wrapped it
    if (clean.contains("```json")) {
        int start = clean.indexOf("```json") + 7;
        int end = clean.indexOf("```", start);
        if (end != -1) {
            clean = clean.mid(start, end - start).trimmed();
        }
    } else if (clean.contains("```")) {
        int start = clean.indexOf("```") + 3;
        int end = clean.indexOf("```", start);
        if (end != -1) {
            clean = clean.mid(start, end - start).trimmed();
        }
    }

    QJsonDocument doc = QJsonDocument::fromJson(clean.toUtf8());
    if (!doc.isObject()) {
        emit statusMessage(QString("⚠️ AI response could not be parsed as mutation JSON:\n%1").arg(clean));
        QJsonObject err;
        err["action"] = "unknown";
        return err;
    }

    return doc.object();
}

} // namespace Seeing
