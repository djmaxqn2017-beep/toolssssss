#include "AppController.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QImageWriter>
#include <QSaveFile>
#include <QStandardPaths>
#include <QtConcurrent/QtConcurrent>
#include <QtMath>

namespace {
static int clamp8(double v) { return qBound(0, qRound(v), 255); }
static double clamp01(double v) { return qBound(0.0, v, 1.0); }

static QImage applyColorRecipe(QImage img, const QVariantMap &s) {
    img = img.convertToFormat(QImage::Format_ARGB32);
    const double exposure = s.value("exposure", 0.0).toDouble();
    const double contrast = s.value("contrast", 0.0).toDouble() / 100.0;
    const double highlights = s.value("highlights", 0.0).toDouble() / 100.0;
    const double shadows = s.value("shadows", 0.0).toDouble() / 100.0;
    const double whites = s.value("whites", 0.0).toDouble() / 100.0;
    const double blacks = s.value("blacks", 0.0).toDouble() / 100.0;
    const double temperature = s.value("temperature", 0.0).toDouble() / 100.0;
    const double tint = s.value("tint", 0.0).toDouble() / 100.0;
    const double saturation = s.value("saturation", 0.0).toDouble() / 100.0;
    const double vibrance = s.value("vibrance", 0.0).toDouble() / 100.0;
    const double gain = qPow(2.0, exposure);

    for (int y = 0; y < img.height(); ++y) {
        auto *line = reinterpret_cast<QRgb*>(img.scanLine(y));
        for (int x = 0; x < img.width(); ++x) {
            const QRgb px = line[x];
            double r = qRed(px) / 255.0;
            double g = qGreen(px) / 255.0;
            double b = qBlue(px) / 255.0;

            r *= gain; g *= gain; b *= gain;
            double l = clamp01(0.2126*r + 0.7152*g + 0.0722*b);
            const double shW = qBound(0.0, (0.60 - l) / 0.60, 1.0);
            const double hiW = qBound(0.0, (l - 0.40) / 0.60, 1.0);
            const double blW = qBound(0.0, (0.22 - l) / 0.22, 1.0);
            const double whW = qBound(0.0, (l - 0.78) / 0.22, 1.0);
            const double tone = shadows * 0.34 * shW + highlights * 0.34 * hiW
                              + blacks * 0.22 * blW + whites * 0.22 * whW;
            r += tone; g += tone; b += tone;

            r += temperature * 0.10 + tint * 0.025;
            g += tint * 0.055;
            b -= temperature * 0.10 + tint * 0.025;

            r = (r - 0.5) * (1.0 + contrast) + 0.5;
            g = (g - 0.5) * (1.0 + contrast) + 0.5;
            b = (b - 0.5) * (1.0 + contrast) + 0.5;

            l = 0.2126*r + 0.7152*g + 0.0722*b;
            const double maxc = qMax(r, qMax(g, b));
            const double minc = qMin(r, qMin(g, b));
            const double chroma = qMax(0.0, maxc - minc);
            const double vibGain = 1.0 + vibrance * (1.0 - qMin(1.0, chroma * 2.2));
            const double satGain = qMax(0.0, 1.0 + saturation) * vibGain;
            r = l + (r-l) * satGain;
            g = l + (g-l) * satGain;
            b = l + (b-l) * satGain;

            line[x] = qRgba(clamp8(r*255.0), clamp8(g*255.0), clamp8(b*255.0), qAlpha(px));
        }
    }
    return img;
}
}

AppController::AppController(QObject *parent) : QObject(parent) {}

QVariantMap AppController::defaultSettings() {
    return {
        {"exposure", 0.0}, {"contrast", 0.0}, {"highlights", 0.0}, {"shadows", 0.0},
        {"whites", 0.0}, {"blacks", 0.0}, {"temperature", 0.0}, {"tint", 0.0},
        {"saturation", 0.0}, {"vibrance", 0.0}
    };
}

QVariantList AppController::images() const {
    QVariantList out;
    out.reserve(m_images.size());
    for (int i = 0; i < m_images.size(); ++i) out.push_back(imageToVariant(m_images.at(i), i));
    return out;
}

int AppController::currentIndex() const { return m_currentIndex; }
QString AppController::currentPreviewUrl() const {
    if (m_currentIndex < 0 || m_currentIndex >= m_images.size()) return {};
    return QUrl::fromLocalFile(m_images.at(m_currentIndex).previewPath).toString();
}
QString AppController::currentName() const {
    if (m_currentIndex < 0 || m_currentIndex >= m_images.size()) return {};
    return m_images.at(m_currentIndex).name;
}
QVariantMap AppController::currentSettings() const {
    if (m_currentIndex < 0 || m_currentIndex >= m_images.size()) return defaultSettings();
    return m_images.at(m_currentIndex).settings;
}
bool AppController::busy() const { return m_busy; }
QString AppController::statusText() const { return m_statusText; }

QString AppController::cacheRoot() const {
    const QString base = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    const QString root = QDir(base).filePath("previews");
    QDir().mkpath(root);
    return root;
}

QString AppController::makePreview(const QString &path, int maxSide, const QString &suffix) const {
    const QFileInfo fi(path);
    const QByteArray keyBytes = (fi.absoluteFilePath() + QString::number(fi.lastModified().toMSecsSinceEpoch())
                                 + QString::number(maxSide)).toUtf8();
    const QString key = QString::fromLatin1(QCryptographicHash::hash(keyBytes, QCryptographicHash::Sha1).toHex());
    const QString outPath = QDir(cacheRoot()).filePath(key + suffix + ".jpg");
    if (QFileInfo::exists(outPath)) return outPath;

    QImageReader reader(path);
    reader.setAutoTransform(true);
    const QSize src = reader.size();
    if (src.isValid() && qMax(src.width(), src.height()) > maxSide) {
        QSize scaled = src;
        scaled.scale(maxSide, maxSide, Qt::KeepAspectRatio);
        reader.setScaledSize(scaled);
    }
    const QImage image = reader.read();
    if (image.isNull()) return {};
    QImageWriter writer(outPath, "jpg");
    writer.setQuality(suffix == "_thumb" ? 86 : 92);
    if (!writer.write(image)) return {};
    return outPath;
}

QVariantMap AppController::imageToVariant(const ImageEntry &entry, int index) const {
    return {
        {"index", index}, {"name", entry.name},
        {"previewUrl", QUrl::fromLocalFile(entry.previewPath).toString()},
        {"thumbUrl", QUrl::fromLocalFile(entry.thumbPath).toString()},
        {"originalPath", entry.originalPath}
    };
}

void AppController::setBusy(bool value) {
    if (m_busy == value) return;
    m_busy = value;
    emit busyChanged();
}

void AppController::setStatusText(const QString &text) {
    if (m_statusText == text) return;
    m_statusText = text;
    emit statusTextChanged();
}

void AppController::importFiles(const QVariantList &urls) {
    if (urls.isEmpty()) return;
    setBusy(true);
    setStatusText(QStringLiteral("Đang tạo preview…"));

    int firstNew = m_images.size();
    for (const QVariant &v : urls) {
        const QUrl url = v.canConvert<QUrl>() ? v.toUrl() : QUrl(v.toString());
        const QString path = url.isLocalFile() ? url.toLocalFile() : v.toString();
        if (path.isEmpty() || !QFileInfo::exists(path)) continue;
        ImageEntry e;
        e.originalPath = QFileInfo(path).absoluteFilePath();
        e.name = QFileInfo(path).fileName();
        e.previewPath = makePreview(e.originalPath, 2200, "_preview");
        e.thumbPath = makePreview(e.originalPath, 260, "_thumb");
        e.settings = defaultSettings();
        if (!e.previewPath.isEmpty()) m_images.push_back(e);
    }
    emit imagesChanged();
    if (m_currentIndex < 0 && firstNew < m_images.size()) selectImage(firstNew);
    setStatusText(QStringLiteral("GPU Preview • CPU Export • Offline"));
    setBusy(false);
}

void AppController::selectImage(int index) {
    if (index < 0 || index >= m_images.size() || index == m_currentIndex) return;
    m_currentIndex = index;
    emit currentIndexChanged();
    emit currentImageChanged();
    emit currentSettingsChanged();
}

void AppController::setSetting(const QString &key, double value) {
    if (m_currentIndex < 0 || m_currentIndex >= m_images.size()) return;
    m_images[m_currentIndex].settings.insert(key, value);
    emit currentSettingsChanged();
}

void AppController::resetCurrentSettings() {
    if (m_currentIndex < 0 || m_currentIndex >= m_images.size()) return;
    m_images[m_currentIndex].settings = defaultSettings();
    emit currentSettingsChanged();
}

void AppController::copySettings() {
    if (m_currentIndex < 0 || m_currentIndex >= m_images.size()) return;
    m_copiedSettings = m_images.at(m_currentIndex).settings;
}

void AppController::pasteSettings() {
    if (m_currentIndex < 0 || m_currentIndex >= m_images.size() || m_copiedSettings.isEmpty()) return;
    m_images[m_currentIndex].settings = m_copiedSettings;
    emit currentSettingsChanged();
}

void AppController::exportCurrent(const QUrl &folderUrl, const QString &format, int quality) {
    if (m_busy || m_currentIndex < 0 || m_currentIndex >= m_images.size()) return;
    const ImageEntry entry = m_images.at(m_currentIndex);
    const QVariantMap settings = entry.settings;
    const QString folder = folderUrl.isLocalFile() ? folderUrl.toLocalFile() : folderUrl.toString();
    if (folder.isEmpty()) { emit errorOccurred(QStringLiteral("Chưa chọn thư mục xuất.")); return; }

    setBusy(true);
    setStatusText(QStringLiteral("Đang xuất từ ảnh gốc…"));

    QtConcurrent::run([this, entry, settings, folder, format, quality]() {
        QImageReader reader(entry.originalPath);
        reader.setAutoTransform(true);
        QImage original = reader.read();
        if (original.isNull()) {
            QMetaObject::invokeMethod(this, [this]() {
                setBusy(false); setStatusText(QStringLiteral("GPU Preview • CPU Export • Offline"));
                emit errorOccurred(QStringLiteral("Không đọc được ảnh gốc để xuất."));
            }, Qt::QueuedConnection);
            return;
        }

        QImage output = applyColorRecipe(original, settings);
        QDir().mkpath(folder);
        QString fmt = format.toLower();
        if (fmt != "png" && fmt != "webp") fmt = "jpg";
        const QString base = QFileInfo(entry.name).completeBaseName();
        const QString outPath = QDir(folder).filePath(base + "_TBRetoch." + fmt);
        QImageWriter writer(outPath, fmt == "jpg" ? QByteArray("jpeg") : fmt.toLatin1());
        writer.setQuality(qBound(70, quality, 100));
        const bool ok = writer.write(output);
        const QFileInfo outInfo(outPath);
        const qint64 bytes = ok ? outInfo.size() : 0;
        const int width = output.width();
        const int height = output.height();
        const QString error = writer.errorString();

        QMetaObject::invokeMethod(this, [this, ok, outPath, bytes, width, height, error]() {
            setBusy(false);
            setStatusText(QStringLiteral("GPU Preview • CPU Export • Offline"));
            if (!ok) emit errorOccurred(error.isEmpty() ? QStringLiteral("Xuất ảnh thất bại.") : error);
            else emit exportFinished(outPath, bytes, width, height);
        }, Qt::QueuedConnection);
    });
}
