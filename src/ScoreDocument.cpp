#include "ScoreDocument.h"
#include <QUndoCommand>
#include <QXmlStreamWriter>
#include <QBuffer>
#include <QIODevice>

class AddNoteCommand : public QUndoCommand {
public:
    AddNoteCommand(ScoreDocument *doc, const NoteEvent &note)
        : m_doc(doc), m_note(note) {
        setText(QString("Add %1%2").arg(note.pitchStep).arg(note.octave));
    }
    void undo() override { m_doc->removeLastNoteInternal(); }
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
    RemoveNoteCommand(ScoreDocument *doc) : m_doc(doc) { setText("Remove Note"); }
    void undo() override { /* will be handled by redo of previous add */ }
    void redo() override { m_doc->removeLastNoteInternal(); }
private:
    ScoreDocument *m_doc;
};

ScoreDocument::ScoreDocument(QObject *parent)
    : QObject(parent), m_undoStack(new QUndoStack(this)) {
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
    n.beatPosition = m_currentBeatPosition;
    m_measures.last().notes.append(n);
    updateBeatPosition();
}

void ScoreDocument::removeLastNote() {
    if (!m_measures.isEmpty() && !m_measures.last().notes.isEmpty()) {
        m_undoStack->push(new RemoveNoteCommand(this));
        emit documentChanged();
    }
}

void ScoreDocument::removeLastNoteInternal() {
    if (!m_measures.isEmpty() && !m_measures.last().notes.isEmpty()) {
        m_measures.last().notes.removeLast();
        updateBeatPosition();
    }
}

void ScoreDocument::addMeasure() {
    m_undoStack->push(new AddMeasureCommand(this));
    emit documentChanged();
}

void ScoreDocument::addMeasureInternal() {
    Measure m;
    m.number = m_measures.size() + 1;
    m_measures.append(m);
    m_currentMeasureIndex = m_measures.size() - 1;
    m_currentBeatPosition = 0;
}

void ScoreDocument::removeLastMeasureInternal() {
    if (m_measures.size() > 1) {
        m_measures.removeLast();
        m_currentMeasureIndex = m_measures.size() - 1;
        m_currentBeatPosition = 0;
    }
}

void ScoreDocument::updateBeatPosition() {
    if (m_measures.isEmpty()) return;
    const Measure &m = m_measures.last();
    m_currentBeatPosition = 0;
    for (const NoteEvent &note : m.notes) {
        m_currentBeatPosition += (4 * 4) / note.duration;
    }
}

void ScoreDocument::clear() {
    m_measures.clear();
    m_currentMeasureIndex = 0;
    m_currentBeatPosition = 0;
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
    writer.writeStartElement("score-part");
    writer.writeAttribute("id", "P1");
    writer.writeTextElement("part-name", "Music");
    writer.writeEndElement();
    writer.writeEndElement();
    
    writer.writeStartElement("part");
    writer.writeAttribute("id", "P1");
    
    for (const Measure &measure : m_measures) {
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
            writer.writeTextElement("sign", "G");
            writer.writeTextElement("line", "2");
            writer.writeEndElement();
            writer.writeEndElement();
        }
        
        for (const NoteEvent &note : measure.notes) {
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
        
        writer.writeEndElement();
    }
    
    writer.writeEndElement();
    writer.writeEndElement();
    writer.writeEndDocument();
    
    return QString::fromUtf8(xmlData);
}