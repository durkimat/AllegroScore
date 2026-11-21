#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"

#include "VerovioScoreRenderer.h"
#include <vrv/toolkit.h>

#include <QImage>
#include <QDebug>
#include <QTemporaryFile>
#include <QFile>
#include <cstring>

VerovioScoreRenderer::VerovioScoreRenderer() {
    toolkit = new vrv::Toolkit();
    vrv::Toolkit* tk = static_cast<vrv::Toolkit*>(toolkit);
    tk->SetOptions("{\"pageWidth\": 210, \"pageHeight\": 297}");
}

VerovioScoreRenderer::~VerovioScoreRenderer() {
    delete static_cast<vrv::Toolkit*>(toolkit);
}

QString VerovioScoreRenderer::renderMusicXmlToSvg(const QString &musicXml) {
    vrv::Toolkit* tk = static_cast<vrv::Toolkit*>(toolkit);
    std::string xml = musicXml.toStdString();

    bool success = tk->LoadData(xml);

    if (success) {
        return QString::fromStdString(tk->RenderToSVG(1));
    }

    qDebug() << "Verovio LoadData failed";
    return QString();
}

QImage VerovioScoreRenderer::renderMusicXmlToImage(const QString &musicXml,
                                                    int width, int height) {
    QString svg = renderMusicXmlToSvg(musicXml);
    if (svg.isEmpty()) {
        return QImage();
    }

    QTemporaryFile tempFile;
    tempFile.setAutoRemove(true);
    if (!tempFile.open()) {
        qDebug() << "Could not create temp file for SVG";
        return QImage();
    }
    tempFile.write(svg.toUtf8());
    tempFile.close();

    QImage image(width, height, QImage::Format_ARGB32);
    image.fill(Qt::white);

    NSVGimage* nsvgImage = nsvgParseFromFile(qPrintable(tempFile.fileName()), "px", 96.0f);
    if (!nsvgImage) {
        qDebug() << "nanosvg: failed to parse SVG file";
        return image;
    }

    NSVGrasterizer* rast = nsvgCreateRasterizer();
    if (!rast) {
        nsvgDelete(nsvgImage);
        return image;
    }

    float scaleX = static_cast<float>(width) / nsvgImage->width;
    float scaleY = static_cast<float>(height) / nsvgImage->height;
    float scale = qMin(scaleX, scaleY);

    unsigned char* bitmap = new unsigned char[width * height * 4];
    std::memset(bitmap, 0xff, width * height * 4);

    nsvgRasterize(rast, nsvgImage, 0, 0, scale,
                  bitmap, width, height, width * 4);

    for (int y = 0; y < height; ++y) {
        QRgb* row = reinterpret_cast<QRgb*>(image.scanLine(y));
        const unsigned char* src = bitmap + (y * width * 4);
        for (int x = 0; x < width; ++x) {
            unsigned char r = src[0];
            unsigned char g = src[1];
            unsigned char b = src[2];
            unsigned char a = src[3];
            row[x] = qRgba(r, g, b, a);
            src += 4;
        }
    }

    delete[] bitmap;
    nsvgDelete(nsvgImage);
    nsvgDeleteRasterizer(rast);

    return image;
}
