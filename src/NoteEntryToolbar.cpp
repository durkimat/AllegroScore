#include "NoteEntryToolbar.h"
#include "ScoreDocument.h"
#include <QAction>
#include <QActionGroup>
#include <QPushButton>
#include <QGridLayout>
#include <QLabel>
#include <QButtonGroup>

NoteEntryToolbar::NoteEntryToolbar(ScoreDocument *doc, QWidget *parent)
    : QToolBar("Note Entry", parent), m_doc(doc) {
    setMovable(false);
    setIconSize(QSize(24, 24));
    
    setupDurationButtons();
    addSeparator();
    setupAccidentalButtons();
    addSeparator();
    setupControlButtons();
    addSeparator();
    setupPitchButtons();
    
    connect(m_doc, &ScoreDocument::documentChanged, this, [this]() {
    });
}

void NoteEntryToolbar::setupDurationButtons() {
    m_durationGroup = new QActionGroup(this);
    m_durationGroup->setExclusive(true);
    
    struct DurationBtn { const char *text; int duration; const char *tooltip; };
    DurationBtn durations[] = {
        { "𝅝", 1, "Whole (1)" },
        { "𝅗𝅥", 2, "Half (2)" },
        { "𝅘𝅥", 4, "Quarter (4)" },
        { "𝅘𝅥𝅘𝅥𝅮", 8, "Eighth (8)" },
        { "𝅘𝅥𝅘𝅥𝅮𝅘𝅥𝅮", 16, "Sixteenth (16)" }
    };
    
    for (const auto &d : durations) {
        QAction *action = new QAction(d.text, this);
        action->setData(d.duration);
        action->setToolTip(d.tooltip);
        action->setCheckable(true);
        action->setActionGroup(m_durationGroup);
        if (d.duration == m_doc->currentDuration()) {
            action->setChecked(true);
        }
        addAction(action);
    }
    
    connect(m_durationGroup, &QActionGroup::triggered, this, &NoteEntryToolbar::onDurationActionTriggered);
    
    QAction *restAction = new QAction("𝄻", this);
    restAction->setToolTip("Rest (toggle)");
    restAction->setCheckable(true);
    restAction->setChecked(m_doc->currentIsRest());
    addAction(restAction);
    connect(restAction, &QAction::toggled, this, &NoteEntryToolbar::onRestToggleClicked);
}

void NoteEntryToolbar::onDurationActionTriggered(QAction *action) {
    m_doc->setCurrentDuration(action->data().toInt());
}

void NoteEntryToolbar::onRestToggleClicked(bool checked) {
    m_doc->setCurrentIsRest(checked);
}

void NoteEntryToolbar::setupAccidentalButtons() {
    m_accidentalGroup = new QActionGroup(this);
    m_accidentalGroup->setExclusive(true);
    
    struct AccidentalBtn { const char *text; const char *value; const char *tooltip; };
    AccidentalBtn accidentals[] = {
        { "♮", "", "Natural" },
        { "♯", "#", "Sharp" },
        { "♭", "b", "Flat" }
    };
    
    for (const auto &a : accidentals) {
        QAction *action = new QAction(a.text, this);
        action->setData(a.value);
        action->setToolTip(a.tooltip);
        action->setCheckable(true);
        action->setActionGroup(m_accidentalGroup);
        if (a.value == m_doc->currentAccidental()) {
            action->setChecked(true);
        }
        addAction(action);
    }
    
    connect(m_accidentalGroup, &QActionGroup::triggered, this, &NoteEntryToolbar::onAccidentalActionTriggered);
}

void NoteEntryToolbar::onAccidentalActionTriggered(QAction *action) {
    m_doc->setCurrentAccidental(action->data().toString());
}

void NoteEntryToolbar::setupPitchButtons() {
    m_pitchWidget = new QWidget(this);
    QGridLayout *grid = new QGridLayout(m_pitchWidget);
    grid->setSpacing(2);
    grid->setContentsMargins(4, 2, 4, 2);
    
    QStringList pitchSteps = {"C", "D", "E", "F", "G", "A", "B"};
    
    for (int octave = 4; octave <= 6; ++octave) {
        QLabel *octaveLabel = new QLabel(QString("Oct %1").arg(octave), m_pitchWidget);
        octaveLabel->setAlignment(Qt::AlignCenter);
        octaveLabel->setStyleSheet("font-size: 10px; color: #666;");
        grid->addWidget(octaveLabel, 0, (octave - 4) * 7);
        
        for (int i = 0; i < 7; ++i) {
            QString pitch = pitchSteps[i] + QString::number(octave);
            QPushButton *btn = new QPushButton(pitchSteps[i], m_pitchWidget);
            btn->setFixedSize(28, 28);
            btn->setToolTip(pitch);
            QString step = pitchSteps[i];
            int oct = octave;
            connect(btn, &QPushButton::clicked, this, [this, step, oct]() {
                onPitchButtonClicked(step, oct);
            });
            grid->addWidget(btn, 1, (octave - 4) * 7 + i);
        }
    }
    
    addWidget(m_pitchWidget);
}

void NoteEntryToolbar::onPitchButtonClicked(const QString &pitchStep, int octave) {
    emit noteRequested(pitchStep, octave, m_doc->currentIsRest());
}

void NoteEntryToolbar::setupControlButtons() {
    QAction *undoAction = new QAction("↶ Undo", this);
    undoAction->setShortcut(QKeySequence::Undo);
    connect(undoAction, &QAction::triggered, this, [this]() {
        m_doc->undoStack()->undo();
        emit undoRequested();
    });
    addAction(undoAction);
    
    QAction *redoAction = new QAction("↷ Redo", this);
    redoAction->setShortcut(QKeySequence::Redo);
    connect(redoAction, &QAction::triggered, this, [this]() {
        m_doc->undoStack()->redo();
        emit redoRequested();
    });
    addAction(redoAction);
    
    QAction *addMeasureAction = new QAction("+ Measure", this);
    addMeasureAction->setShortcut(Qt::Key_Plus);
    connect(addMeasureAction, &QAction::triggered, this, [this]() {
        m_doc->addMeasure();
        emit addMeasureRequested();
    });
    addAction(addMeasureAction);
}