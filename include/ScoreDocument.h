#ifndef SCORE_DOCUMENT_H
#define SCORE_DOCUMENT_H

#include <QString>
#include <QVector>
#include <QStack>
#include <QUndoStack>

struct NoteEvent {
    QString pitchStep;      // A-G
    int octave;             // 4-6
    QString accidental;     // "", "#", "b", "natural"
    int duration;           // 1=whole, 2=half, 4=quarter, 8=eighth, 16=sixteenth
    bool isRest;
    int measureIndex;
    int voiceIndex;
    int beatPosition;       // in divisions (quarter = 4 divisions)
};

struct Measure {
    int number;
    int divisions = 4;
    int beats = 4;
    int beatType = 4;
    QVector<NoteEvent> notes;
};

class ScoreDocument : public QObject {
    Q_OBJECT

public:
    explicit ScoreDocument(QObject *parent = nullptr);
    
    // Document state
    QString generateMusicXml() const;
    void clear();
    
    // Note manipulation
    void addNote(const NoteEvent &note);
    void removeLastNote();
    void addMeasure();
    
    // Internal methods for undo commands
    void addNoteInternal(const NoteEvent &note);
    void removeLastNoteInternal();
    void addMeasureInternal();
    void removeLastMeasureInternal();
    void updateBeatPosition();
    
    // Selection state for toolbar
    int currentDuration() const { return m_currentDuration; }
    void setCurrentDuration(int d) { m_currentDuration = d; }
    
    QString currentAccidental() const { return m_currentAccidental; }
    void setCurrentAccidental(const QString &a) { m_currentAccidental = a; }
    
    int currentOctave() const { return m_currentOctave; }
    void setCurrentOctave(int o) { m_currentOctave = o; }
    
    bool currentIsRest() const { return m_currentIsRest; }
    void setCurrentIsRest(bool r) { m_currentIsRest = r; }
    
    // Undo/Redo
    QUndoStack *undoStack() { return m_undoStack; }
    
    // Current cursor position
    int currentMeasureIndex() const { return m_currentMeasureIndex; }
    int currentBeatPosition() const { return m_currentBeatPosition; }
    
signals:
    void documentChanged();

private:
    QVector<Measure> m_measures;
    int m_currentMeasureIndex = 0;
    int m_currentBeatPosition = 0;
    
    // Toolbar state
    int m_currentDuration = 4;       // quarter note default
    QString m_currentAccidental = "";
    int m_currentOctave = 4;
    bool m_currentIsRest = false;
    
    QUndoStack *m_undoStack;
};

#endif // SCORE_DOCUMENT_H