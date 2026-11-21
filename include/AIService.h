#ifndef AISERVICE_H
#define AISERVICE_H

#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class AIService : public QObject {
    Q_OBJECT

public:
    explicit AIService(const QString &apiKey, QObject *parent = nullptr);
    void sendMessage(const QString &message, const QString &scoreContext);

signals:
    void responseReceived(const QString &response);
    void errorOccurred(const QString &error);

private:
    QString m_apiKey;
    QNetworkAccessManager m_networkManager;
};

#endif // AISERVICE_H
