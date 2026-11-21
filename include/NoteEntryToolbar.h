#ifndef NOTE_ENTRY_TOOLBAR_H
#define NOTE_ENTRY_TOOLBAR_H

#include <QToolBar>
#include <QAction>
#include <QActionGroup>
#include <QWidget>
#include <QComboBox>

class ScoreDocument;

class NoteEntryToolbar : public QToolBar {
    Q_OBJECT

public:
    explicit NoteEntryToolbar(ScoreDocument *doc, QWidget *parent = nullptr);
    
signals:
    void noteRequested(const QString &pitchStep, int octave, bool isRest);
    void addMeasureRequested();
    void undoRequested();
    void redoRequested();

private slots:
    void onDurationActionTriggered(QAction *action);
    void onAccidentalActionTriggered(QAction *action);
    void onPitchButtonClicked(const QString &pitchStep, int octave);
    void onRestToggleClicked(bool checked);

private:
    void setupDurationButtons();
    void setupAccidentalButtons();
    void setupPitchButtons();
    void setupControlButtons();
    void setupPartSelector();
    
    ScoreDocument *m_doc;
    QActionGroup *m_durationGroup;
    QActionGroup *m_accidentalGroup;
    QWidget *m_pitchWidget;
    QComboBox *m_partComboBox;
};

#endif // NOTE_ENTRY_TOOLBAR_H