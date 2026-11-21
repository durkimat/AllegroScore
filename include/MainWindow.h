#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QLineEdit>
#include <QSplitter>
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include "ScoreRenderer.h"
#include "VerovioScoreRenderer.h"
#include "AIService.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onSendClicked();
    void onAIResponse(const QString &response);
    void onAIError(const QString &error);

private:
    void setupUi();

    QSplitter *mainSplitter;
    QWidget *scoreArea;
    QWidget *chatFrame;
    QTextEdit *chatHistory;
    QTextEdit *chatInput;
    QLineEdit *apiKeyInput;
    QLabel *scoreView;
    ScoreRenderer *scoreRenderer;
    AIService *aiService;
    QString m_currentMusicXml;
};

#endif // MAINWINDOW_H
