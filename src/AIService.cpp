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

    QString model = m_model;
    if (model.isEmpty() || model == "gpt-4o") {
        model = "auto";
    }

    QUrl url(m_baseUrl + "chat/completions");
    QJsonObject json;
    json["model"] = model;
    json["stream"] = false;

    QJsonArray messages;
    QJsonObject systemMsg;
    systemMsg["role"] = "system";
    systemMsg["content"] = "You are an expert music composition assistant. When the user asks to create, edit, arrange, or harmonize music, you MUST include a valid MusicXML 3.1 document inside a fenced code block marked as 'musicxml' alongside a brief explanation. Example:\n```musicxml\n<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<score-partwise version=\"3.1\">\n  ...\n</score-partwise>\n```";
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
        QByteArray rawData = reply->readAll();
        int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(rawData);
            if (doc.isNull()) {
                emit errorOccurred("Invalid JSON response: " + QString::fromUtf8(rawData));
                reply->deleteLater();
                return;
            }
            QJsonObject response = doc.object();

            if (response.contains("error") && response["error"].isObject()) {
                QJsonObject errorObj = response["error"].toObject();
                QString errorMsg = errorObj["message"].toString("Unknown API error");
                emit errorOccurred("API Error: " + errorMsg);
                reply->deleteLater();
                return;
            }

            QJsonArray choices = response["choices"].toArray();
            if (choices.isEmpty() || choices[0].isNull()) {
                emit errorOccurred("API returned empty choices: " + QString::fromUtf8(QJsonDocument(response).toJson(QJsonDocument::Compact)));
                reply->deleteLater();
                return;
            }

            QJsonObject choice = choices[0].toObject();
            QJsonObject messageObj = choice["message"].toObject();
            QJsonValue contentVal = messageObj["content"];

            QString content;
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

            if (content.isEmpty()) {
                content = QString::fromUtf8(QJsonDocument(response).toJson(QJsonDocument::Compact));
            }

            emit responseReceived(content);
        } else {
            QString errorDetail = QString("HTTP %1: %2").arg(httpStatus).arg(reply->errorString());
            if (!rawData.isEmpty()) {
                errorDetail += "\nRaw response: " + QString::fromUtf8(rawData);
            }
            emit errorOccurred(errorDetail);
        }
        reply->deleteLater();
    });
}