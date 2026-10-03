#pragma once
#include <QImage>
#include <QMap>
#include <QVector>
#include <QVector3D>
#include <QRectF>
#include <QStringList>
#include <QVariantMap>
#include <memory>

struct FaceAnalysis {
    QRectF bounds; // normalized, before crop/rotate
    QVector<QVector3D> landmarks; // canonical MediaPipe 478 topology
    double confidence = 0;
};
struct PortraitAnalysis {
    QVector<FaceAnalysis> faces;
    QMap<QString,QImage> masks; // grayscale confidence, original aspect ratio
    QString backend;
};

QStringList semanticMaskNames();
bool hasSemanticSettings(const QVariantMap &settings);
QVariantMap semanticDefaults();
bool isSemanticKey(const QString &key);
QString canonicalSettingKey(const QString &key);
std::shared_ptr<PortraitAnalysis> analysePortrait(const QImage &source, const QString &cache,
                                               bool needDepth = false, bool needSky = false);
QImage semanticMask(const PortraitAnalysis &analysis, const QString &name, QSize size, int face = -1);
QImage applyPortraitRecipe(QImage image, const QVariantMap &settings, const PortraitAnalysis &analysis);
