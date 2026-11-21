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
#include "ScoreDocument.h"

class NoteEntryToolbar;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onSendClicked();
    void onAIResponse(const QString &response);
    void onAIError(const QString &error);
    void loadSettings();
    void saveSettings();
    void onNoteRequested(const QString &pitchStep, int octave, bool isRest);
    void onAddMeasureRequested();
    void onDocumentChanged();
    void keyPressEvent(QKeyEvent *event) override;

private:
    void setupUi();
    void renderScore();

    QSplitter *mainSplitter;
    QWidget *scoreArea;
    QWidget *chatFrame;
    QTextEdit *chatHistory;
    QTextEdit *chatInput;
    QLineEdit *apiKeyInput;
    QLineEdit *baseUrlInput;
    QLineEdit *modelInput;
    QLabel *scoreView;
    ScoreRenderer *scoreRenderer;
    AIService *aiService;
    ScoreDocument *scoreDocument;
    NoteEntryToolbar *noteToolbar;
    QString m_currentMusicXml;
};

#endif // MAINWINDOW_H
