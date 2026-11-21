#ifndef AISERVICE_H
#define AISERVICE_H

#include <QString>
#include <QObject>
#include <QSettings>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrl>

class AIService : public QObject {
    Q_OBJECT

public:
    explicit AIService(QObject *parent = nullptr);
    ~AIService();

    // Configuration accessors
    QString baseUrl() const;
    void setBaseUrl(const QString &url);
    QString model() const;
    void setModel(const QString &model);
    QString apiKey() const;
    void setApiKey(const QString &key);

    // Public API
    void sendMessage(const QString &message, const QString &scoreContext);

signals:
    void responseReceived(const QString &response);
    void errorOccurred(const QString &error);

private:
    void loadConfig();
    void saveConfig();
    QString m_baseUrl;
    QString m_model;
    QString m_apiKey;
    QSettings *m_settings;
    QNetworkAccessManager m_networkManager;
};

#endif // AISERVICE_H