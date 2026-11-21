#include "AIService.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QUrlQuery>

AIService::AIService(QObject *parent)
    : QObject(parent), m_settings(new QSettings(QSettings::Format::IniFormat,
                                                 QSettings::Scope::UserScope,
                                                 QStringLiteral("AllegroScore"),
                                                 QStringLiteral("AllegroScore"), this)) {
    loadConfig();
}

AIService::~AIService() = default;

QString AIService::baseUrl() const { return m_baseUrl; }
void AIService::setBaseUrl(const QString &url) { m_baseUrl = url; }

QString AIService::model() const { return m_model; }
void AIService::setModel(const QString &model) { m_model = model; }

QString AIService::apiKey() const { return m_apiKey; }
void AIService::setApiKey(const QString &key) { m_apiKey = key; }

void AIService::loadConfig() {
    m_baseUrl = m_settings->value("baseUrl", "https://openai.com").toString();
    if (!m_baseUrl.endsWith('/')) m_baseUrl.append('/');
    m_model = m_settings->value("model", "gpt-4o").toString();
    m_apiKey = m_settings->value("apiKey", "").toString();
}

void AIService::saveConfig() {
    m_settings->setValue("baseUrl", m_baseUrl);
    m_settings->setValue("model", m_model);
    m_settings->setValue("apiKey", m_apiKey);
}

void AIService::sendMessage(const QString &message, const QString &scoreContext) {
    if (m_apiKey.isEmpty()) {
        emit errorOccurred("No API key configured.");
        return;
    }

    QUrl url(m_baseUrl + "chat/completions");
    QJsonObject json;
    json["model"] = m_model.isEmpty() ? "gpt-4o" : m_model;

    QJsonArray messages;
    QJsonObject systemMsg;
    systemMsg["role"] = "system";
    systemMsg["content"] = "You are an expert music composition assistant. Help the user with their score, suggest music theory, style adaptation, and error checking. Be concise.";
    messages.append(systemMsg);

    QJsonObject userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = message + "\n\nScore Context (MusicXML excerpt):\n" + scoreContext;
    messages.append(userMsg);

    json["messages"] = messages;
    json["temperature"] = 0.7;

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + m_apiKey).toUtf8());

    QNetworkReply *reply = m_networkManager.post(request, QJsonDocument(json).toJson());
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonObject response = QJsonDocument::fromJson(reply->readAll()).object();
            QString content = response["choices"].toArray()[0].toObject()["message"].toObject()["content"].toString();
            emit responseReceived(content);
        } else {
            emit errorOccurred(reply->errorString());
        }
        reply->deleteLater();
    });
}