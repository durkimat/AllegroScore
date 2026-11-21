#include "ScoreRenderer.h"
#include <QLabel>
#include <QVBoxLayout>

QString MockScoreRenderer::renderMusicXmlToSvg(const QString &musicXml) {
    Q_UNUSED(musicXml);
    return QString("<svg width=\"400\" height=\"200\" xmlns=\"http://www.w3.org/2000/svg\">"
                   "<rect width=\"100%\" height=\"100%\" fill=\"#f0f0f0\"/>"
                   "<text x=\"200\" y=\"100\" text-anchor=\"middle\" font-size=\"18\" fill=\"#666\">"
                   "Notation Engine Not Available</text>"
                   "</svg>");
}
