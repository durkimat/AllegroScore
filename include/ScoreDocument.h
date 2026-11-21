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
    int partIndex;          // 0=Soprano, 1=Alto, 2=Tenor, 3=Bass
    int voiceIndex;
    int beatPosition;       // in divisions (quarter = 4 divisions)
};

struct Measure {
    int number;
    int divisions = 4;
    int beats = 4;
    int beatType = 4;
    // Notes per part (SATB)
    QVector<QVector<NoteEvent>> notesPerPart;
};

class ScoreDocument : public QObject {
    Q_OBJECT

public:
    explicit ScoreDocument(QObject *parent = nullptr);
    
    // Document state
    QString generateMusicXml() const;
    void clear();
    void loadMusicXml(const QString &xml);
    
    // Note manipulation
    void addNote(const NoteEvent &note);
    void removeLastNote(int partIndex = -1);
    void addMeasure();
    
    // Internal methods for undo commands
    void addNoteInternal(const NoteEvent &note);
    void removeLastNoteInternal(int partIndex);
    void addMeasureInternal();
    void removeLastMeasureInternal();
    void updateBeatPosition(int partIndex);
    
    // Selection state for toolbar
    int currentDuration() const { return m_currentDuration; }
    void setCurrentDuration(int d) { m_currentDuration = d; }
    
    QString currentAccidental() const { return m_currentAccidental; }
    void setCurrentAccidental(const QString &a) { m_currentAccidental = a; }
    
    int currentOctave() const { return m_currentOctave; }
    void setCurrentOctave(int o) { m_currentOctave = o; }
    
    bool currentIsRest() const { return m_currentIsRest; }
    void setCurrentIsRest(bool r) { m_currentIsRest = r; }
    
    int currentPartIndex() const { return m_currentPartIndex; }
    void setCurrentPartIndex(int p) { m_currentPartIndex = p; }
    
    // Undo/Redo
    QUndoStack *undoStack() { return m_undoStack; }
    
    // Current cursor position
    int currentMeasureIndex() const { return m_currentMeasureIndex; }
    int currentBeatPosition(int partIndex = -1) const;
    
    // Part names for SATB
    static QStringList partNames();
    static QStringList partClefSigns();
    static QStringList partClefLines();
    
signals:
    void documentChanged();
    void errorOccurred(const QString &error);

private:
    QVector<Measure> m_measures;
    int m_currentMeasureIndex = 0;
    QVector<int> m_currentBeatPositions;  // per part
    
    // Toolbar state
    int m_currentDuration = 4;       // quarter note default
    QString m_currentAccidental = "";
    int m_currentOctave = 4;
    bool m_currentIsRest = false;
    int m_currentPartIndex = 0;      // 0=Soprano, 1=Alto, 2=Tenor, 3=Bass
    
    QUndoStack *m_undoStack;
};

#endif // SCORE_DOCUMENT_H