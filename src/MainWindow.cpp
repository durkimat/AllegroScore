#include "MainWindow.h"
#include "NoteEntryToolbar.h"
#include <QPushButton>
#include <QSettings>
#include <QKeyEvent>
#include <QMessageBox>
#include <QRegularExpression>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), aiService(nullptr), scoreDocument(nullptr), noteToolbar(nullptr) {
    scoreRenderer = new VerovioScoreRenderer();
    scoreDocument = new ScoreDocument(this);
    setupUi();
    loadSettings();
    
    connect(scoreDocument, &ScoreDocument::documentChanged, this, &MainWindow::onDocumentChanged);
    connect(scoreDocument, &ScoreDocument::errorOccurred, this, &MainWindow::onScoreError);
    connect(noteToolbar, &NoteEntryToolbar::noteRequested, this, &MainWindow::onNoteRequested);
    connect(noteToolbar, &NoteEntryToolbar::addMeasureRequested, this, &MainWindow::onAddMeasureRequested);
    
    setWindowTitle("AllegroScore");
    resize(1200, 800);
    
    renderScore();
}

MainWindow::~MainWindow() {
    delete scoreRenderer;
    if (aiService) delete aiService;
}

void MainWindow::loadSettings() {
    QSettings settings(QSettings::Format::IniFormat,
                       QSettings::Scope::UserScope,
                       QStringLiteral("AllegroScore"),
                       QStringLiteral("AllegroScore"));
    apiKeyInput->setText(settings.value("apiKey", "").toString());
    baseUrlInput->setText(settings.value("baseUrl", "https://openai.com").toString());
    modelInput->setText(settings.value("model", "auto").toString());
}

void MainWindow::saveSettings() {
    QSettings settings(QSettings::Format::IniFormat,
                       QSettings::Scope::UserScope,
                       QStringLiteral("AllegroScore"),
                       QStringLiteral("AllegroScore"));
    settings.setValue("apiKey", apiKeyInput->text());
    settings.setValue("baseUrl", baseUrlInput->text());
    settings.setValue("model", modelInput->text());
}

void MainWindow::setupUi() {
    mainSplitter = new QSplitter(Qt::Horizontal, this);

    // Score Area
    scoreArea = new QWidget(this);
    QVBoxLayout *scoreLayout = new QVBoxLayout(scoreArea);
    scoreLayout->setContentsMargins(0, 0, 0, 0);
    scoreLayout->setSpacing(0);

    // Note Entry Toolbar
    noteToolbar = new NoteEntryToolbar(scoreDocument, this);
    scoreLayout->addWidget(noteToolbar);

    scoreView = new QLabel("Loading score...", scoreArea);
    scoreView->setAlignment(Qt::AlignCenter);
    scoreView->setStyleSheet("background-color: #ffffff; border: 1px solid #cccccc; font-size: 18px;");
    scoreView->setMinimumSize(600, 400);
    scoreView->setWordWrap(true);
    scoreView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    scoreLayout->addWidget(scoreView);

    // Chat Frame
    chatFrame = new QWidget(this);
    QVBoxLayout *chatLayout = new QVBoxLayout(chatFrame);
    
    apiKeyInput = new QLineEdit(chatFrame);
    apiKeyInput->setEchoMode(QLineEdit::Password);
    apiKeyInput->setPlaceholderText("API Key...");

    baseUrlInput = new QLineEdit(chatFrame);
    baseUrlInput->setPlaceholderText("Base URL (e.g. https://openai.com or http://192.168.1.xxx)");

    modelInput = new QLineEdit(chatFrame);
    modelInput->setPlaceholderText("Model (e.g. gpt-4o, auto)");

    chatHistory = new QTextEdit(chatFrame);
    chatHistory->setReadOnly(true);
    chatHistory->setPlaceholderText("AI Agent interaction will appear here...");
    
    chatInput = new QTextEdit(chatFrame);
    chatInput->setMaximumHeight(100);
    chatInput->setPlaceholderText("Ask the AI Assistant...");

    QPushButton *sendButton = new QPushButton("Send", chatFrame);
    connect(sendButton, &QPushButton::clicked, this, &MainWindow::onSendClicked);

    chatLayout->addWidget(new QLabel("API Key"));
    chatLayout->addWidget(apiKeyInput);
    chatLayout->addWidget(new QLabel("Base URL"));
    chatLayout->addWidget(baseUrlInput);
    chatLayout->addWidget(new QLabel("Model"));
    chatLayout->addWidget(modelInput);
    chatLayout->addWidget(new QLabel("AI Assistant"));
    chatLayout->addWidget(chatHistory);
    chatLayout->addWidget(chatInput);
    chatLayout->addWidget(sendButton);

    mainSplitter->addWidget(scoreArea);
    mainSplitter->addWidget(chatFrame);

    mainSplitter->setStretchFactor(0, 7);
    mainSplitter->setStretchFactor(1, 3);

    setCentralWidget(mainSplitter);
}

void MainWindow::onSendClicked() {
    QString apiKey = apiKeyInput->text().trimmed();
    if (apiKey.isEmpty()) {
        chatHistory->append("<b>System:</b> Please enter a valid API Key.\n");
        return;
    }

    saveSettings();

    if (!aiService) {
        aiService = new AIService(this);
        connect(aiService, &AIService::responseReceived, this, &MainWindow::onAIResponse);
        connect(aiService, &AIService::errorOccurred, this, &MainWindow::onAIError);
    }

    aiService->setApiKey(apiKeyInput->text());
    aiService->setBaseUrl(baseUrlInput->text());
    aiService->setModel(modelInput->text());

    QString message = chatInput->toPlainText().trimmed();
    if (message.isEmpty()) return;

    chatHistory->append("<b>User:</b> " + message + "\n");
    chatInput->clear();

    aiService->sendMessage(message, m_currentMusicXml);
}

void MainWindow::onAIResponse(const QString &response) {
    chatHistory->append("<b>AI:</b> " + response + "\n");
    
    // Extract musicxml fenced code block from response
    QString musicXml = extractMusicXmlFromResponse(response);
    if (!musicXml.isEmpty()) {
        scoreDocument->loadMusicXml(musicXml);
        chatHistory->append("<b>System:</b> Loaded MusicXML from AI response.\n");
    }
}

QString MainWindow::extractMusicXmlFromResponse(const QString &response) {
    // Look for ```musicxml ... ``` fenced code block
    QRegularExpression regex("```musicxml\\s*\\n([\\s\\S]*?)\\n```");
    QRegularExpressionMatch match = regex.match(response);
    if (match.hasMatch()) {
        return match.captured(1).trimmed();
    }
    // Also try case insensitive
    regex.setPattern("```musicxml\\s*\\n([\\s\\S]*?)\\n```");
    regex.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
    match = regex.match(response);
    if (match.hasMatch()) {
        return match.captured(1).trimmed();
    }
    return QString();
}

void MainWindow::onAIError(const QString &error) {
    chatHistory->append("<b>Error:</b> " + error + "\n");
}

void MainWindow::onScoreError(const QString &error) {
    chatHistory->append("<b>Score Error:</b> " + error + "\n");
}

void MainWindow::onNoteRequested(const QString &pitchStep, int octave, bool isRest) {
    NoteEvent note;
    note.pitchStep = pitchStep;
    note.octave = octave;
    note.accidental = scoreDocument->currentAccidental();
    note.duration = scoreDocument->currentDuration();
    note.isRest = isRest;
    note.measureIndex = scoreDocument->currentMeasureIndex();
    note.partIndex = scoreDocument->currentPartIndex();
    note.voiceIndex = 0;
    note.beatPosition = scoreDocument->currentBeatPosition();
    
    scoreDocument->addNote(note);
}

void MainWindow::onAddMeasureRequested() {
    // Already handled by the document
}

void MainWindow::onDocumentChanged() {
    renderScore();
}

void MainWindow::renderScore() {
    m_currentMusicXml = scoreDocument->generateMusicXml();
    QImage scoreImage = scoreRenderer->renderMusicXmlToImage(m_currentMusicXml, 800, 400);
    if (!scoreImage.isNull()) {
        scoreView->setPixmap(QPixmap::fromImage(scoreImage));
    } else {
        scoreView->setText("Score rendering failed. Check Verovio integration.");
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    static QStringList pitchKeys = {"C", "D", "E", "F", "G", "A", "B"};
    
    if (event->key() >= Qt::Key_A && event->key() <= Qt::Key_G) {
        int idx = event->key() - Qt::Key_A;
        if (idx < pitchKeys.size()) {
            onNoteRequested(pitchKeys[idx], scoreDocument->currentOctave(), scoreDocument->currentIsRest());
            event->accept();
            return;
        }
    }
    
    if (event->key() == Qt::Key_Backspace || event->key() == Qt::Key_Delete) {
        scoreDocument->removeLastNote();
        event->accept();
        return;
    }
    
    if (event->key() == Qt::Key_Plus || event->key() == Qt::Key_Equal) {
        scoreDocument->addMeasure();
        event->accept();
        return;
    }
    
    if (event->matches(QKeySequence::Undo)) {
        scoreDocument->undoStack()->undo();
        event->accept();
        return;
    }
    
    if (event->matches(QKeySequence::Redo)) {
        scoreDocument->undoStack()->redo();
        event->accept();
        return;
    }
    
    QMainWindow::keyPressEvent(event);
}
