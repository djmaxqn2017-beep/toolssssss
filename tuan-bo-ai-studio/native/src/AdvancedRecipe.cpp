#include "AdvancedRecipe.h"
#include <QColor>
#include <QTransform>
#include <QtMath>
#include <array>

namespace {
const std::array<const char *, 8> bands = {"red", "orange", "yellow", "green", "cyan", "blue", "purple", "magenta"};
const std::array<double, 8> centers = {0, 30, 60, 120, 180, 220, 260, 310};
int byte(double value) { return qBound(0, qRound(value), 255); }
double unit(double value) { return qBound(0.0, value, 1.0); }
double curve(double x, const std::array<double,6> &points) {
    const double p = unit(x) * 5;
    const int i = qMin(4, int(p));
    return unit(points[i] + (points[i+1] - points[i]) * (p-i));
}
}

bool isGeometryKey(const QString &key) {
    return key.startsWith("crop") || key == "rotation" || key == "straighten" || key.startsWith("flip");
}

QVariantMap advancedDefaults() {
    QVariantMap values;
    for (const char *key : {"cropLeft", "cropRight", "cropTop", "cropBottom", "rotation", "straighten", "flipH", "flipV",
         "sharpness", "denoise", "vignette", "grain", "curveShadows", "curveDarks", "curveLights", "curveHighlights"}) values.insert(key, 0.0);
    for (const char *band : bands)
        for (const char *effect : {"Hue", "Sat", "Lum"}) values.insert(QString("hsl_") + band + effect, 0.0);
    return values;
}

bool hasAdvancedSettings(const QVariantMap &settings) {
    const QVariantMap defaults = advancedDefaults();
    for (auto it = defaults.cbegin(); it != defaults.cend(); ++it)
        if (!qFuzzyIsNull(settings.value(it.key()).toDouble())) return true;
    return false;
}

QImage applyAdvancedRecipe(QImage image, const QVariantMap &s) {
    if (!hasAdvancedSettings(s)) return image;
    image = image.convertToFormat(QImage::Format_ARGB32);
    const double sharp = s.value("sharpness").toDouble() / 100;
    const double noise = s.value("denoise").toDouble() / 100;
    if (sharp > 0 || noise > 0) {
        // Edge-aware 3x3 denoise and unsharp mask, with an immutable input plane.
        const QImage source = image;
        for (int y = 0; y < image.height(); ++y) {
            QRgb *dst = reinterpret_cast<QRgb *>(image.scanLine(y));
            for (int x = 0; x < image.width(); ++x) {
                const QRgb center = source.pixel(x, y);
                double sums[3] = {0, 0, 0}, weight = 0;
                for (int j = -1; j <= 1; ++j) {
                    const auto *row = reinterpret_cast<const QRgb *>(source.constScanLine(qBound(0, y+j, source.height()-1)));
                    for (int i = -1; i <= 1; ++i) {
                        const QRgb p = row[qBound(0, x+i, source.width()-1)];
                        const double delta = qAbs(qRed(p)-qRed(center)) + qAbs(qGreen(p)-qGreen(center)) + qAbs(qBlue(p)-qBlue(center));
                        const double w = noise > 0 ? qExp(-delta / (8 + noise * 75)) : 1;
                        sums[0] += qRed(p)*w; sums[1] += qGreen(p)*w; sums[2] += qBlue(p)*w; weight += w;
                    }
                }
                const double r = qRed(center), g = qGreen(center), b = qBlue(center);
                const double amount = sharp * 1.8 - noise;
                dst[x] = qRgba(byte(r+(r-sums[0]/weight)*amount), byte(g+(g-sums[1]/weight)*amount), byte(b+(b-sums[2]/weight)*amount), qAlpha(center));
            }
        }
    }
    bool hslEnabled = false;
    for (const char *band : bands) for (const char *effect : {"Hue", "Sat", "Lum"})
        hslEnabled |= s.value(QString("hsl_")+band+effect).toDouble() != 0;
    std::array<double,8> hues{}, saturations{}, luminances{};
    for (int band = 0; band < 8; ++band) {
        const QString key = QString("hsl_") + bands[band];
        hues[band] = s.value(key+"Hue").toDouble();
        saturations[band] = s.value(key+"Sat").toDouble();
        luminances[band] = s.value(key+"Lum").toDouble();
    }
    const bool curveEnabled = s.value("curveShadows").toDouble() != 0 || s.value("curveDarks").toDouble() != 0
        || s.value("curveLights").toDouble() != 0 || s.value("curveHighlights").toDouble() != 0;
    const std::array<double,6> curvePoints = {0, .2 + s.value("curveShadows").toDouble() * .0015,
        .4 + s.value("curveDarks").toDouble() * .0015, .6 + s.value("curveLights").toDouble() * .0015,
        .8 + s.value("curveHighlights").toDouble() * .0015, 1};
    const double vignette = s.value("vignette").toDouble()/100;
    const double grain = s.value("grain").toDouble()/100;
    if (hslEnabled || curveEnabled || vignette != 0 || grain > 0) {
        for (int y = 0; y < image.height(); ++y) {
            auto *row = reinterpret_cast<QRgb *>(image.scanLine(y));
            for (int x = 0; x < image.width(); ++x) {
                const QRgb p = row[x];
                double r = qRed(p)/255.0, g = qGreen(p)/255.0, b = qBlue(p)/255.0;
                if (hslEnabled) {
                    float h, sat, lum;
                    QColor::fromRgbF(r, g, b).getHslF(&h, &sat, &lum);
                    if (h >= 0 && sat > .001) {
                        double hueDelta = 0, satDelta = 0, lumDelta = 0;
                        for (int band = 0; band < 8; ++band) {
                            double distance = qAbs(h*360 - centers[band]);
                            distance = qMin(distance, 360-distance);
                            const double w = qMax(0.0, 1-distance/45);
                            hueDelta += hues[band] * w * .3;
                            satDelta += saturations[band] * w / 100;
                            lumDelta += luminances[band] * w / 100;
                        }
                        double hue = std::fmod(h + hueDelta/360 + 1, 1);
                        const QColor adjusted = QColor::fromHslF(hue, unit(sat*(1+satDelta)), unit(lum+lumDelta*.35));
                        r = adjusted.redF(); g = adjusted.greenF(); b = adjusted.blueF();
                    }
                }
                if (curveEnabled) { r = curve(r,curvePoints); g = curve(g,curvePoints); b = curve(b,curvePoints); }
                if (vignette != 0) {
                    const double nx = (x+.5)/image.width()*2-1, ny = (y+.5)/image.height()*2-1;
                    const double edge = unit((qSqrt(nx*nx+ny*ny)-.35)/.95);
                    const double amount = vignette * edge*edge * .7;
                    if (amount > 0) { r *= 1-amount; g *= 1-amount; b *= 1-amount; }
                    else { r += -amount*(1-r); g += -amount*(1-g); b += -amount*(1-b); }
                }
                if (grain > 0) {
                    quint32 hash = quint32(x)*374761393u + quint32(y)*668265263u + 0x9e3779b9u;
                    hash = (hash^(hash>>13))*1274126177u;
                    const double value = ((hash>>8)&65535)/65535.0-.5;
                    r += value*grain*.12; g += value*grain*.12; b += value*grain*.12;
                }
                row[x] = qRgba(byte(r*255),byte(g*255),byte(b*255),qAlpha(p));
            }
        }
    }
    const int left = qBound(0, qRound(image.width()*s.value("cropLeft").toDouble()/100), image.width()-1);
    const int top = qBound(0, qRound(image.height()*s.value("cropTop").toDouble()/100), image.height()-1);
    const int width = qMax(1, image.width()-left-qRound(image.width()*s.value("cropRight").toDouble()/100));
    const int height = qMax(1, image.height()-top-qRound(image.height()*s.value("cropBottom").toDouble()/100));
    if (left || top || width != image.width() || height != image.height()) image = image.copy(left,top,width,height);
    if (s.value("flipH").toBool() || s.value("flipV").toBool()) image = image.mirrored(s.value("flipH").toBool(),s.value("flipV").toBool());
    const double angle = s.value("rotation").toDouble()*90 + s.value("straighten").toDouble();
    if (angle != 0) image = image.transformed(QTransform().rotate(angle),Qt::SmoothTransformation);
    return image;
}
