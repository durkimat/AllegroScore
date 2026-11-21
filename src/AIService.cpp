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
    m_model = m_settings->value("model", "auto").toString();
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
    json["model"] = m_model.isEmpty() ? "auto" : m_model;
    json["stream"] = false;

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
            QByteArray rawData = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(rawData);
            QJsonObject response = doc.object();

            QString content;
            QJsonArray choices = response["choices"].toArray();
            if (!choices.isEmpty()) {
                QJsonObject choice = choices[0].toObject();
                QJsonObject messageObj = choice["message"].toObject();
                QJsonValue contentVal = messageObj["content"];

                if (contentVal.isString()) {
                    content = contentVal.toString();
                } else if (contentVal.isArray()) {
                    QJsonArray parts = contentVal.toArray();
                    QStringList textParts;
                    for (const QJsonValue &part : parts) {
                        QJsonObject partObj = part.toObject();
                        if (partObj["type"].toString() == "text" && partObj["text"].isString()) {
                            textParts.append(partObj["text"].toString());
                        }
                    }
                    content = textParts.join("\n");
                }

                if (content.isEmpty()) {
                    if (choice["text"].isString()) {
                        content = choice["text"].toString();
                    } else if (messageObj["reasoning_content"].isString()) {
                        content = messageObj["reasoning_content"].toString();
                    }
                }
            }

            if (content.isEmpty()) {
                content = QString::fromUtf8(QJsonDocument(response).toJson(QJsonDocument::Compact));
            }

            emit responseReceived(content);
        } else {
            emit errorOccurred(reply->errorString());
        }
        reply->deleteLater();
    });
}