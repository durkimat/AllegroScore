#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QSplitter>
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include "ScoreRenderer.h"
#include "VerovioScoreRenderer.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void setupUi();

    QSplitter *mainSplitter;
    QWidget *scoreArea;
    QWidget *chatFrame;
    QTextEdit *chatHistory;
    QTextEdit *chatInput;
    QLabel *scoreView;
    ScoreRenderer *scoreRenderer;
};

#endif // MAINWINDOW_H
