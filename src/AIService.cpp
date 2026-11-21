#include "AIService.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>

AIService::AIService(const QString &apiKey, QObject *parent)
    : QObject(parent), m_apiKey(apiKey) {}

void AIService::sendMessage(const QString &message, const QString &scoreContext) {
    QNetworkRequest request(QUrl("https://api.openai.com/v1/chat/completions")); // Or Claude equivalent
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + m_apiKey).toUtf8());

    QJsonObject json;
    json["model"] = "gpt-4o"; // Placeholder
    QJsonArray messages;
    
    QJsonObject systemMsg;
    systemMsg["role"] = "system";
    systemMsg["content"] = "You are an expert music composition assistant. Assist the user with their score.";
    messages.append(systemMsg);

    QJsonObject userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = message + "\n\nScore Context: " + scoreContext;
    messages.append(userMsg);
    
    json["messages"] = messages;

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
