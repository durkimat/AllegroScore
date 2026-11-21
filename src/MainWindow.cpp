#include "MainWindow.h"
#include <QPushButton>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), aiService(nullptr) {
    scoreRenderer = new VerovioScoreRenderer();
    setupUi();
    setWindowTitle("AllegroScore");
    resize(1200, 800);
}

MainWindow::~MainWindow() {
    delete scoreRenderer;
    if (aiService) delete aiService;
}

void MainWindow::setupUi() {
    mainSplitter = new QSplitter(Qt::Horizontal, this);

    // Score Area
    scoreArea = new QWidget(this);
    QVBoxLayout *scoreLayout = new QVBoxLayout(scoreArea);
    scoreLayout->setContentsMargins(0, 0, 0, 0);

    scoreView = new QLabel("Loading score...", scoreArea);
    scoreView->setAlignment(Qt::AlignCenter);
    scoreView->setStyleSheet("background-color: #ffffff; border: 1px solid #cccccc; font-size: 18px;");
    scoreView->setMinimumSize(600, 400);
    scoreView->setWordWrap(true);
    scoreLayout->addWidget(scoreView);

    // Load a demo MusicXML snippet
    m_currentMusicXml = R"(
        <?xml version="1.0" encoding="UTF-8"?>
        <score-partwise version="3.1">
            <part-list>
                <score-part id="P1">
                    <part-name>Music</part-name>
                </score-part>
            </part-list>
            <part id="P1">
                <measure number="1">
                    <attributes>
                        <divisions>1</divisions>
                        <key><fifths>0</fifths></key>
                        <time><beats>4</beats><beat-type>4</beat-type></time>
                        <clef><sign>G</sign><line>2</line></clef>
                    </attributes>
                    <note>
                        <pitch><step>C</step><octave>4</octave></pitch>
                        <duration>4</duration>
                        <type>whole</type>
                    </note>
                </measure>
            </part>
        </score-partwise>
    )";

    QImage scoreImage = scoreRenderer->renderMusicXmlToImage(m_currentMusicXml, 800, 400);
    if (!scoreImage.isNull()) {
        scoreView->setPixmap(QPixmap::fromImage(scoreImage));
    } else {
        scoreView->setText("Score rendering failed. Check Verovio integration.");
    }

    // Chat Frame
    chatFrame = new QWidget(this);
    QVBoxLayout *chatLayout = new QVBoxLayout(chatFrame);
    
    apiKeyInput = new QLineEdit(chatFrame);
    apiKeyInput->setEchoMode(QLineEdit::Password);
    apiKeyInput->setPlaceholderText("API Key...");

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

    if (!aiService) {
        aiService = new AIService(apiKey, this);
        connect(aiService, &AIService::responseReceived, this, &MainWindow::onAIResponse);
        connect(aiService, &AIService::errorOccurred, this, &MainWindow::onAIError);
    }

    QString message = chatInput->toPlainText().trimmed();
    if (message.isEmpty()) return;

    chatHistory->append("<b>User:</b> " + message + "\n");
    chatInput->clear();

    aiService->sendMessage(message, m_currentMusicXml);
}

void MainWindow::onAIResponse(const QString &response) {
    chatHistory->append("<b>AI:</b> " + response + "\n");
}

void MainWindow::onAIError(const QString &error) {
    chatHistory->append("<b>Error:</b> " + error + "\n");
}
