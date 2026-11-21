#ifndef VERovioSCORERENDERER_H
#define VERovioSCORERENDERER_H

#include "ScoreRenderer.h"
#include <QImage>

class VerovioScoreRenderer : public ScoreRenderer {
public:
    VerovioScoreRenderer();
    ~VerovioScoreRenderer() override;

    QString renderMusicXmlToSvg(const QString &musicXml) override;
    bool isAvailable() const override { return true; }

    // Render MusicXML directly to a QImage via SVG intermediate
    QImage renderMusicXmlToImage(const QString &musicXml, int width, int height);

private:
    void *toolkit; // opaque pointer to vrv::Toolkit
};

#endif // VERovioSCORERENDERER_H
