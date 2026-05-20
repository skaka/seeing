#include "settings_dialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QSettings>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCoreApplication>

namespace Seeing {

SettingsDialog::SettingsDialog(EditorController& controller, QWidget* parent)
    : QDialog(parent)
    , m_controller(controller)
{
    setWindowTitle("AI Copilot Settings");
    resize(480, 360);
    setupUI();
    loadSettings();
    applyTheme();
}

void SettingsDialog::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    mainLayout->setSpacing(12);

    // ── Title Header ────────────────────────────────────────────────────────
    auto* header = new QLabel("AI Copilot Configuration", this);
    header->setObjectName("headerLabel");
    mainLayout->addWidget(header);

    // ── Dual Engine Selectors ────────────────────────────────────────────────
    auto* selectorLayout = new QFormLayout();

    m_vlmCombo = new QComboBox(this);
    m_vlmCombo->addItem("Local Qwen2-VL (Offline)", "marlin");
    m_vlmCombo->addItem("OpenAI GPT-4o Vision (Cloud)", "openai");
    m_vlmCombo->addItem("Gemini 2.5 Flash (Cloud)", "gemini");
    m_vlmCombo->addItem("Mock VLM (Dummy)", "dummy");

    m_engineCombo = new QComboBox(this);
    m_engineCombo->addItem("Dummy AI (MVP Mock)", "dummy");
    m_engineCombo->addItem("OpenAI (GPT Cloud)", "openai");
    m_engineCombo->addItem("Gemini (Google Cloud)", "gemini");
    m_engineCombo->addItem("Ollama (Local Models)", "ollama");

    selectorLayout->addRow("VLM Indexer AI (Media Analysis):", m_vlmCombo);
    selectorLayout->addRow("NLE Montage Manager (Chat Copilot):", m_engineCombo);
    mainLayout->addLayout(selectorLayout);

    // ── Stacked Widget for Panels ───────────────────────────────────────────
    m_stackedWidget = new QStackedWidget(this);
    mainLayout->addWidget(m_stackedWidget);

    // 1. Dummy Panel
    auto* dummyWidget = new QWidget(this);
    auto* dummyLayout = new QVBoxLayout(dummyWidget);
    auto* dummyInfo = new QLabel("This engine simulates a local AI model for local testing.\nNo API keys or endpoints required.", this);
    dummyInfo->setWordWrap(true);
    dummyInfo->setStyleSheet("color: #858585; font-style: italic;");
    dummyLayout->addWidget(dummyInfo);
    dummyLayout->addStretch();
    m_stackedWidget->addWidget(dummyWidget);

    // 2. OpenAI Panel
    auto* openaiWidget = new QWidget(this);
    auto* openaiForm = new QFormLayout(openaiWidget);
    openaiForm->setContentsMargins(0, 10, 0, 10);
    m_openaiKey = new QLineEdit(this);
    m_openaiKey->setEchoMode(QLineEdit::Password);
    m_openaiKey->setPlaceholderText("sk-proj-...");
    m_openaiModel = new QLineEdit(this);
    m_openaiModel->setPlaceholderText("gpt-4o");
    openaiForm->addRow("API Secret Key:", m_openaiKey);
    openaiForm->addRow("Model Identifier:", m_openaiModel);
    m_stackedWidget->addWidget(openaiWidget);

    // 3. Gemini Panel
    auto* geminiWidget = new QWidget(this);
    auto* geminiForm = new QFormLayout(geminiWidget);
    geminiForm->setContentsMargins(0, 10, 0, 10);
    m_geminiKey = new QLineEdit(this);
    m_geminiKey->setEchoMode(QLineEdit::Password);
    m_geminiKey->setPlaceholderText("AIzaSy...");
    m_geminiModel = new QLineEdit(this);
    m_geminiModel->setPlaceholderText("gemini-2.5-flash");
    geminiForm->addRow("Gemini API Key:", m_geminiKey);
    geminiForm->addRow("Model Identifier:", m_geminiModel);
    m_stackedWidget->addWidget(geminiWidget);

    // 4. Ollama Panel
    auto* ollamaWidget = new QWidget(this);
    auto* ollamaForm = new QFormLayout(ollamaWidget);
    ollamaForm->setContentsMargins(0, 10, 0, 10);
    m_ollamaUrl = new QLineEdit(this);
    m_ollamaUrl->setPlaceholderText("http://localhost:11434");
    m_ollamaModel = new QLineEdit(this);
    m_ollamaModel->setPlaceholderText("llama3");
    ollamaForm->addRow("Ollama Endpoint URL:", m_ollamaUrl);
    ollamaForm->addRow("Model Name:", m_ollamaModel);
    m_stackedWidget->addWidget(ollamaWidget);

    // 5. Marlin Panel
    auto* marlinWidget = new QWidget(this);
    auto* marlinForm = new QFormLayout(marlinWidget);
    marlinForm->setContentsMargins(0, 10, 0, 10);
    m_hfToken = new QLineEdit(this);
    m_hfToken->setEchoMode(QLineEdit::Password);
    m_hfToken->setPlaceholderText("hf_...");
    marlinForm->addRow("Hugging Face Token:", m_hfToken);
    m_stackedWidget->addWidget(marlinWidget);

    // ── Test Connection Panel ───────────────────────────────────────────────
    auto* testLayout = new QHBoxLayout();
    m_testBtn = new QPushButton("Test Connection", this);
    m_testBtn->setObjectName("secondaryBtn");
    m_testStatus = new QLabel("", this);
    m_testStatus->setObjectName("statusLabel");
    testLayout->addWidget(m_testBtn);
    testLayout->addWidget(m_testStatus);
    testLayout->addStretch();
    mainLayout->addLayout(testLayout);

    mainLayout->addSpacing(10);

    // ── Bottom Save/Cancel Buttons ──────────────────────────────────────────
    auto* btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    m_cancelBtn = new QPushButton("Cancel", this);
    m_cancelBtn->setObjectName("secondaryBtn");
    m_saveBtn = new QPushButton("Save Settings", this);
    m_saveBtn->setObjectName("primaryBtn");
    btnLayout->addWidget(m_cancelBtn);
    btnLayout->addWidget(m_saveBtn);
    mainLayout->addLayout(btnLayout);

    // ── Connect Signals ─────────────────────────────────────────────────────
    connect(m_engineCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsDialog::onEngineChanged);
    connect(m_testBtn, &QPushButton::clicked, this, &SettingsDialog::onTestConnection);
    connect(m_saveBtn, &QPushButton::clicked, this, &SettingsDialog::onSave);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void SettingsDialog::loadSettings()
{
    QSettings settings;

    QString vlm = settings.value("ai/vlm_engine_type", "marlin").toString();
    int vlmIndex = m_vlmCombo->findData(vlm);
    if (vlmIndex != -1) {
        m_vlmCombo->setCurrentIndex(vlmIndex);
    }

    QString engine = settings.value("ai/engine_type", "dummy").toString();
    int index = m_engineCombo->findData(engine);
    if (index != -1) {
        m_engineCombo->setCurrentIndex(index);
        onEngineChanged(index);
    }

    m_openaiKey->setText(settings.value("ai/openai_key", "").toString());
    m_openaiModel->setText(settings.value("ai/openai_model", "gpt-4o").toString());

    m_geminiKey->setText(settings.value("ai/gemini_key", "").toString());
    m_geminiModel->setText(settings.value("ai/gemini_model", "gemini-2.5-flash").toString());

    m_ollamaUrl->setText(settings.value("ai/ollama_url", "http://localhost:11434").toString());
    m_ollamaModel->setText(settings.value("ai/ollama_model", "llama3").toString());

    m_hfToken->setText(settings.value("ai/hf_token", "").toString());
}

void SettingsDialog::onEngineChanged(int index)
{
    Q_UNUSED(index);
    QString currentData = m_engineCombo->currentData().toString();
    if (currentData == "dummy")      m_stackedWidget->setCurrentIndex(0);
    else if (currentData == "openai") m_stackedWidget->setCurrentIndex(1);
    else if (currentData == "gemini") m_stackedWidget->setCurrentIndex(2);
    else if (currentData == "ollama") m_stackedWidget->setCurrentIndex(3);
    m_testStatus->setText("");
}

void SettingsDialog::onSave()
{
    QSettings settings;
    QString selectedVlm = m_vlmCombo->currentData().toString();
    QString selectedEngine = m_engineCombo->currentData().toString();

    settings.setValue("ai/vlm_engine_type", selectedVlm);
    settings.setValue("ai/engine_type", selectedEngine);

    settings.setValue("ai/openai_key", m_openaiKey->text().trimmed());
    settings.setValue("ai/openai_model", m_openaiModel->text().trimmed().isEmpty() ? "gpt-4o" : m_openaiModel->text().trimmed());

    settings.setValue("ai/gemini_key", m_geminiKey->text().trimmed());
    settings.setValue("ai/gemini_model", m_geminiModel->text().trimmed().isEmpty() ? "gemini-2.5-flash" : m_geminiModel->text().trimmed());

    settings.setValue("ai/ollama_url", m_ollamaUrl->text().trimmed().isEmpty() ? "http://localhost:11434" : m_ollamaUrl->text().trimmed());
    settings.setValue("ai/ollama_model", m_ollamaModel->text().trimmed().isEmpty() ? "llama3" : m_ollamaModel->text().trimmed());

    settings.setValue("ai/hf_token", m_hfToken->text().trimmed());

    // Tell the Controller to switch to the new active engine
    m_controller.setAiEngine(selectedEngine);

    accept();
}

void SettingsDialog::onTestConnection()
{
    m_testStatus->setText("⌛ Testing connection...");
    m_testStatus->setStyleSheet("color: #858585;");
    m_testBtn->setEnabled(false);

    // Force UI update
    QCoreApplication::processEvents();

    QString engine = m_engineCombo->currentData().toString();
    QNetworkAccessManager manager;
    QUrl url;
    QNetworkRequest request;

    if (engine == "dummy") {
        m_testStatus->setText("✅ Mock engine online.");
        m_testStatus->setStyleSheet("color: #89d185;");
        m_testBtn->setEnabled(true);
        return;
    }
    else if (engine == "openai") {
        QString key = m_openaiKey->text().trimmed();
        if (key.isEmpty()) {
            m_testStatus->setText("❌ API key required");
            m_testStatus->setStyleSheet("color: #f48771;");
            m_testBtn->setEnabled(true);
            return;
        }
        url = QUrl("https://api.openai.com/v1/models");
        request.setUrl(url);
        request.setRawHeader("Authorization", ("Bearer " + key).toUtf8());
    }
    else if (engine == "gemini") {
        QString key = m_geminiKey->text().trimmed();
        if (key.isEmpty()) {
            m_testStatus->setText("❌ API key required");
            m_testStatus->setStyleSheet("color: #f48771;");
            m_testBtn->setEnabled(true);
            return;
        }
        QString model = m_geminiModel->text().trimmed();
        if (model.isEmpty()) model = "gemini-2.5-flash";
        url = QUrl(QString("https://generativelanguage.googleapis.com/v1beta/models/%1?key=%2").arg(model, key));
        request.setUrl(url);
    }
    else if (engine == "ollama") {
        QString baseUrl = m_ollamaUrl->text().trimmed();
        if (baseUrl.isEmpty()) baseUrl = "http://localhost:11434";
        if (baseUrl.endsWith("/")) baseUrl.chop(1);
        url = QUrl(baseUrl + "/api/tags");
        request.setUrl(url);
    }
    else if (engine == "marlin") {
        url = QUrl("http://127.0.0.1:8012/health");
        request.setUrl(url);
    }

    QNetworkReply* reply = manager.get(request);

    // Asynchronous network test via QEventLoop
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    m_testBtn->setEnabled(true);

    if (reply->error() == QNetworkReply::NoError) {
        m_testStatus->setText("✅ Connection Successful!");
        m_testStatus->setStyleSheet("color: #89d185;");
    } else {
        m_testStatus->setText(QString("❌ Fail: %1").arg(reply->errorString()));
        m_testStatus->setStyleSheet("color: #f48771;");
    }

    reply->deleteLater();
}

void SettingsDialog::applyTheme()
{
    setStyleSheet(R"(
        QDialog {
            background-color: #1e1e1e;
            color: #d4d4d4;
            font-family: 'Segoe UI', Arial, sans-serif;
            font-size: 13px;
        }
        QLabel {
            color: #cccccc;
        }
        QLabel#headerLabel {
            font-size: 16px;
            font-weight: bold;
            color: #ffffff;
            border-bottom: 2px solid #007acc;
            padding-bottom: 8px;
            margin-bottom: 5px;
        }
        QLabel#statusLabel {
            font-size: 12px;
        }
        QLineEdit {
            background-color: #2d2d2d;
            border: 1px solid #3c3c3c;
            border-radius: 3px;
            color: #d4d4d4;
            padding: 5px;
        }
        QLineEdit:focus {
            border: 1px solid #007acc;
            background-color: #333333;
        }
        QComboBox {
            background-color: #2d2d2d;
            border: 1px solid #3c3c3c;
            border-radius: 3px;
            color: #d4d4d4;
            padding: 5px;
            min-width: 150px;
        }
        QComboBox:on {
            border: 1px solid #007acc;
        }
        QComboBox QAbstractItemView {
            background-color: #252526;
            color: #d4d4d4;
            selection-background-color: #094771;
            border: 1px solid #3c3c3c;
        }
        QPushButton#primaryBtn {
            background-color: #0e639c;
            color: #ffffff;
            border: none;
            border-radius: 3px;
            padding: 6px 14px;
            font-weight: bold;
        }
        QPushButton#primaryBtn:hover {
            background-color: #1177bb;
        }
        QPushButton#primaryBtn:pressed {
            background-color: #0c5282;
        }
        QPushButton#secondaryBtn {
            background-color: #3e3e42;
            color: #cccccc;
            border: none;
            border-radius: 3px;
            padding: 6px 14px;
        }
        QPushButton#secondaryBtn:hover {
            background-color: #4e4e52;
            color: #ffffff;
        }
        QPushButton#secondaryBtn:pressed {
            background-color: #2d2d30;
        }
    )");
}

} // namespace Seeing
