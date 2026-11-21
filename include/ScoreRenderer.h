#ifndef SCORE_RENDERER_H
#define SCORE_RENDERER_H

#include <QString>
#include <QImage>

class ScoreRenderer {
public:
    virtual ~ScoreRenderer() = default;

    // Render MusicXML content into an SVG string
    virtual QString renderMusicXmlToSvg(const QString &musicXml) = 0;

    // Render MusicXML content directly to a QImage
    virtual QImage renderMusicXmlToImage(const QString &musicXml,
                                         int width, int height) = 0;

    // Check if the underlying engine is available
    virtual bool isAvailable() const = 0;
};

#endif // SCORE_RENDERER_H
