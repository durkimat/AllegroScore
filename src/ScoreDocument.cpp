#include "ScoreDocument.h"
#include <QUndoCommand>
#include <QXmlStreamWriter>
#include <QBuffer>
#include <QIODevice>
#include <QDomDocument>
#include <QRegularExpression>

class AddNoteCommand : public QUndoCommand {
public:
    AddNoteCommand(ScoreDocument *doc, const NoteEvent &note)
        : m_doc(doc), m_note(note) {
        setText(QString("Add %1%2 to %3").arg(note.pitchStep).arg(note.octave).arg(ScoreDocument::partNames()[note.partIndex]));
    }
    void undo() override { m_doc->removeLastNoteInternal(m_note.partIndex); }
    void redo() override { m_doc->addNoteInternal(m_note); }
private:
    ScoreDocument *m_doc;
    NoteEvent m_note;
};

class AddMeasureCommand : public QUndoCommand {
public:
    AddMeasureCommand(ScoreDocument *doc) : m_doc(doc) { setText("Add Measure"); }
    void undo() override { m_doc->removeLastMeasureInternal(); }
    void redo() override { m_doc->addMeasureInternal(); }
private:
    ScoreDocument *m_doc;
};

class RemoveNoteCommand : public QUndoCommand {
public:
    RemoveNoteCommand(ScoreDocument *doc, int partIndex) : m_doc(doc), m_partIndex(partIndex) { setText("Remove Note"); }
    void undo() override { /* will be handled by redo of previous add */ }
    void redo() override { m_doc->removeLastNoteInternal(m_partIndex); }
private:
    ScoreDocument *m_doc;
    int m_partIndex;
};

ScoreDocument::ScoreDocument(QObject *parent)
    : QObject(parent), m_undoStack(new QUndoStack(this)) {
    m_currentBeatPositions.resize(4, 0);
    addMeasureInternal();
}

void ScoreDocument::addNote(const NoteEvent &note) {
    m_undoStack->push(new AddNoteCommand(this, note));
    emit documentChanged();
}

void ScoreDocument::addNoteInternal(const NoteEvent &note) {
    if (m_measures.isEmpty()) {
        addMeasureInternal();
    }
    NoteEvent n = note;
    n.measureIndex = m_currentMeasureIndex;
    n.partIndex = qBound(0, n.partIndex, 3);
    n.beatPosition = m_currentBeatPositions[n.partIndex];
    
    // Ensure the notesPerPart vector has enough parts
    while (m_measures.last().notesPerPart.size() <= n.partIndex) {
        m_measures.last().notesPerPart.append(QVector<NoteEvent>());
    }
    
    m_measures.last().notesPerPart[n.partIndex].append(n);
    updateBeatPosition(n.partIndex);
}

void ScoreDocument::removeLastNote(int partIndex) {
    int idx = (partIndex >= 0) ? partIndex : m_currentPartIndex;
    if (!m_measures.isEmpty() && m_measures.last().notesPerPart.size() > idx && !m_measures.last().notesPerPart[idx].isEmpty()) {
        m_undoStack->push(new RemoveNoteCommand(this, idx));
        emit documentChanged();
    }
}

void ScoreDocument::removeLastNoteInternal(int partIndex) {
    if (!m_measures.isEmpty() && m_measures.last().notesPerPart.size() > partIndex && !m_measures.last().notesPerPart[partIndex].isEmpty()) {
        m_measures.last().notesPerPart[partIndex].removeLast();
        updateBeatPosition(partIndex);
    }
}

void ScoreDocument::addMeasure() {
    m_undoStack->push(new AddMeasureCommand(this));
    emit documentChanged();
}

void ScoreDocument::addMeasureInternal() {
    Measure m;
    m.number = m_measures.size() + 1;
    m.notesPerPart.resize(4);  // SATB
    m_measures.append(m);
    m_currentMeasureIndex = m_measures.size() - 1;
    for (int i = 0; i < 4; ++i) {
        m_currentBeatPositions[i] = 0;
    }
}

void ScoreDocument::removeLastMeasureInternal() {
    if (m_measures.size() > 1) {
        m_measures.removeLast();
        m_currentMeasureIndex = m_measures.size() - 1;
        for (int i = 0; i < 4; ++i) {
            m_currentBeatPositions[i] = 0;
        }
    }
}

void ScoreDocument::updateBeatPosition(int partIndex) {
    if (m_measures.isEmpty()) return;
    const Measure &m = m_measures.last();
    if (m.notesPerPart.size() <= partIndex) return;
    
    m_currentBeatPositions[partIndex] = 0;
    for (const NoteEvent &note : m.notesPerPart[partIndex]) {
        m_currentBeatPositions[partIndex] += (4 * m.divisions) / note.duration;
    }
}

int ScoreDocument::currentBeatPosition(int partIndex) const {
    if (partIndex >= 0 && partIndex < m_currentBeatPositions.size()) {
        return m_currentBeatPositions[partIndex];
    }
    return m_currentBeatPositions[m_currentPartIndex];
}

void ScoreDocument::clear() {
    m_measures.clear();
    m_currentMeasureIndex = 0;
    m_currentBeatPositions.fill(0);
    m_undoStack->clear();
    addMeasureInternal();
    emit documentChanged();
}

QString ScoreDocument::generateMusicXml() const {
    QByteArray xmlData;
    QBuffer buffer(&xmlData);
    buffer.open(QIODevice::WriteOnly);
    
    QXmlStreamWriter writer(&buffer);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeStartElement("score-partwise");
    writer.writeAttribute("version", "3.1");
    
    writer.writeStartElement("part-list");
    QStringList names = partNames();
    QStringList clefSigns = partClefSigns();
    QStringList clefLines = partClefLines();
    for (int i = 0; i < 4; ++i) {
        writer.writeStartElement("score-part");
        writer.writeAttribute("id", QString("P%1").arg(i + 1));
        writer.writeTextElement("part-name", names[i]);
        writer.writeEndElement();
    }
    writer.writeEndElement();
    
    QStringList clefSignList = partClefSigns();
    QStringList clefLineList = partClefLines();
    
    for (int partIdx = 0; partIdx < 4; ++partIdx) {
        writer.writeStartElement("part");
        writer.writeAttribute("id", QString("P%1").arg(partIdx + 1));
        
        for (int measureIdx = 0; measureIdx < m_measures.size(); ++measureIdx) {
            const Measure &measure = m_measures[measureIdx];
            writer.writeStartElement("measure");
            writer.writeAttribute("number", QString::number(measure.number));
            
            if (measure.number == 1) {
                writer.writeStartElement("attributes");
                writer.writeTextElement("divisions", QString::number(measure.divisions));
                writer.writeStartElement("key");
                writer.writeTextElement("fifths", "0");
                writer.writeEndElement();
                writer.writeStartElement("time");
                writer.writeTextElement("beats", QString::number(measure.beats));
                writer.writeTextElement("beat-type", QString::number(measure.beatType));
                writer.writeEndElement();
                writer.writeStartElement("clef");
                writer.writeTextElement("sign", clefSignList[partIdx]);
                writer.writeTextElement("line", clefLineList[partIdx]);
                writer.writeEndElement();
                writer.writeEndElement();
            }
            
            if (measure.notesPerPart.size() > partIdx) {
                for (const NoteEvent &note : measure.notesPerPart[partIdx]) {
                    writer.writeStartElement("note");
                    
                    if (note.isRest) {
                        writer.writeEmptyElement("rest");
                    } else {
                        writer.writeStartElement("pitch");
                        writer.writeTextElement("step", note.pitchStep);
                        writer.writeTextElement("octave", QString::number(note.octave));
                        if (!note.accidental.isEmpty()) {
                            writer.writeTextElement("alter", 
                                note.accidental == "#" ? "1" : 
                                note.accidental == "b" ? "-1" : "0");
                        }
                        writer.writeEndElement();
                    }
                    
                    int durationDivs = (4 * measure.divisions) / note.duration;
                    writer.writeTextElement("duration", QString::number(durationDivs));
                    
                    QString typeStr;
                    switch (note.duration) {
                        case 1: typeStr = "whole"; break;
                        case 2: typeStr = "half"; break;
                        case 4: typeStr = "quarter"; break;
                        case 8: typeStr = "eighth"; break;
                        case 16: typeStr = "16th"; break;
                        default: typeStr = "quarter";
                    }
                    writer.writeTextElement("type", typeStr);
                    
                    if (note.accidental == "natural") {
                        writer.writeStartElement("accidental");
                        writer.writeCharacters("natural");
                        writer.writeEndElement();
                    }
                    
                    writer.writeEndElement();
                }
            }
            
            writer.writeEndElement();
        }
        
        writer.writeEndElement();
    }
    
    writer.writeEndElement();
    writer.writeEndElement();
    writer.writeEndDocument();
    
    return QString::fromUtf8(xmlData);
}

void ScoreDocument::loadMusicXml(const QString &xml) {
    QDomDocument doc;
    QString errorMsg;
    int errorLine, errorCol;
    if (!doc.setContent(xml, &errorMsg, &errorLine, &errorCol)) {
        emit errorOccurred(QString("Failed to parse MusicXML: %1 at line %2 col %3").arg(errorMsg).arg(errorLine).arg(errorCol));
        return;
    }
    
    QDomElement root = doc.documentElement();
    if (root.tagName() != "score-partwise") {
        emit errorOccurred("Invalid MusicXML: root element is not score-partwise");
        return;
    }
    
    clear();
    
    QDomNodeList partNodes = root.elementsByTagName("part");
    int maxMeasures = 0;
    
    for (int partIdx = 0; partIdx < partNodes.size() && partIdx < 4; ++partIdx) {
        QDomElement partElem = partNodes.at(partIdx).toElement();
        QDomNodeList measureNodes = partElem.elementsByTagName("measure");
        
        maxMeasures = qMax(maxMeasures, measureNodes.size());
        
        // Ensure we have enough measures
        while (m_measures.size() < measureNodes.size()) {
            addMeasureInternal();
        }
        
        for (int measureIdx = 0; measureIdx < measureNodes.size(); ++measureIdx) {
            QDomElement measureElem = measureNodes.at(measureIdx).toElement();
            QDomNodeList noteNodes = measureElem.elementsByTagName("note");
            
            for (int noteIdx = 0; noteIdx < noteNodes.size(); ++noteIdx) {
                QDomElement noteElem = noteNodes.at(noteIdx).toElement();
                NoteEvent note;
                note.measureIndex = measureIdx;
                note.partIndex = partIdx;
                note.voiceIndex = 0;
                
                QDomElement pitchElem = noteElem.firstChildElement("pitch");
                if (!pitchElem.isNull()) {
                    note.isRest = false;
                    note.pitchStep = pitchElem.firstChildElement("step").text();
                    note.octave = pitchElem.firstChildElement("octave").text().toInt();
                    QDomElement alterElem = pitchElem.firstChildElement("alter");
                    if (!alterElem.isNull()) {
                        int alter = alterElem.text().toInt();
                        if (alter > 0) note.accidental = "#";
                        else if (alter < 0) note.accidental = "b";
                        else note.accidental = "natural";
                    }
                } else {
                    note.isRest = true;
                }
                
                note.duration = 4; // default quarter
                QDomElement typeElem = noteElem.firstChildElement("type");
                if (!typeElem.isNull()) {
                    QString typeStr = typeElem.text();
                    if (typeStr == "whole") note.duration = 1;
                    else if (typeStr == "half") note.duration = 2;
                    else if (typeStr == "quarter") note.duration = 4;
                    else if (typeStr == "eighth") note.duration = 8;
                    else if (typeStr == "16th") note.duration = 16;
                }
                
                // Add the note directly without undo stack
                NoteEvent n = note;
                n.beatPosition = m_currentBeatPositions[partIdx];
                while (m_measures[measureIdx].notesPerPart.size() <= partIdx) {
                    m_measures[measureIdx].notesPerPart.append(QVector<NoteEvent>());
                }
                m_measures[measureIdx].notesPerPart[partIdx].append(n);
                updateBeatPosition(partIdx);
            }
        }
    }
    
    m_currentMeasureIndex = qMax(0, maxMeasures - 1);
    emit documentChanged();
}

QStringList ScoreDocument::partNames() {
    return {"Soprano", "Alto", "Tenor", "Bass"};
}

QStringList ScoreDocument::partClefSigns() {
    return {"G", "G", "G", "F"};
}

QStringList ScoreDocument::partClefLines() {
    return {"2", "2", "2", "4"};
}