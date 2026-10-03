#include "AppController.h"
#include "ImageDecoder.h"
#include <QFileDialog>
#include <QSaveFile>
#include <QTemporaryFile>
#include <QJSValue>

#include <QCryptographicHash>
#include <QDir>
#include <QDateTime>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QImageWriter>
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
    const double clarity = s.value("clarity", 0.0).toDouble() / 100.0;
    const double dehaze = s.value("dehaze", 0.0).toDouble() / 100.0;
    const double fade = s.value("fade", 0.0).toDouble() / 100.0;
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

            l = 0.2126*r + 0.7152*g + 0.0722*b;
            const double midWeight = qBound(0.0, 1.0 - qAbs(l - 0.5) * 2.0, 1.0);
            const double clarityGain = 1.0 + clarity * 0.32 * midWeight;
            r = l + (r - l) * clarityGain;
            g = l + (g - l) * clarityGain;
            b = l + (b - l) * clarityGain;

            const double hazeContrast = 1.0 + dehaze * 0.42;
            r = (r - 0.5) * hazeContrast + 0.5;
            g = (g - 0.5) * hazeContrast + 0.5;
            b = (b - 0.5) * hazeContrast + 0.5;

            if (fade != 0.0) {
                const double amount = qBound(-1.0, fade, 1.0);
                if (amount >= 0.0) {
                    r = r * (1.0 - amount * 0.18) + amount * 0.045;
                    g = g * (1.0 - amount * 0.18) + amount * 0.045;
                    b = b * (1.0 - amount * 0.18) + amount * 0.045;
                } else {
                    const double a = -amount;
                    r = (r - 0.04 * a) * (1.0 + 0.14 * a);
                    g = (g - 0.04 * a) * (1.0 + 0.14 * a);
                    b = (b - 0.04 * a) * (1.0 + 0.14 * a);
                }
            }

            line[x] = qRgba(clamp8(r*255.0), clamp8(g*255.0), clamp8(b*255.0), qAlpha(px));
        }
    }
    return img;
}
}

AppController::AppController(QObject *parent) : QObject(parent) { QImageReader::setAllocationLimit(512); }
AppController::~AppController() { m_workers.waitForDone(); }

QVariantMap AppController::defaultSettings() {
    return {
        {"exposure", 0.0}, {"contrast", 0.0}, {"highlights", 0.0}, {"shadows", 0.0},
        {"whites", 0.0}, {"blacks", 0.0}, {"temperature", 0.0}, {"tint", 0.0},
        {"saturation", 0.0}, {"vibrance", 0.0}, {"clarity", 0.0}, {"dehaze", 0.0},
        {"fade", 0.0}
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
bool AppController::canUndo() const {
    return m_currentIndex >= 0 && m_currentIndex < m_images.size() && !m_images.at(m_currentIndex).undoStack.isEmpty();
}
bool AppController::canRedo() const {
    return m_currentIndex >= 0 && m_currentIndex < m_images.size() && !m_images.at(m_currentIndex).redoStack.isEmpty();
}

QString AppController::cacheRoot() const {
    const QString overridePath = qEnvironmentVariable("TBRETOCH_CACHE_ROOT");
    const QString base = overridePath.isEmpty() ? QStandardPaths::writableLocation(QStandardPaths::CacheLocation) : overridePath;
    QString root = QDir(base).filePath("previews");
    if (!base.isEmpty() && QDir().mkpath(root)) {
        QTemporaryFile probe(QDir(root).filePath(".write-test-XXXXXX"));
        if (probe.open()) return root;
    }
    // A blocked profile cache must not prevent all photo imports.
    root = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation)).filePath("TBRetoch/previews");
    QDir().mkpath(root);
    return root;
}

QString AppController::makePreview(const QString &path, const QImage &image, int maxSide, const QString &suffix, QString *error) const {
    const QFileInfo fi(path);
    const QByteArray keyBytes = (fi.absoluteFilePath() + QString::number(fi.lastModified().toMSecsSinceEpoch())
                                 + QString::number(fi.size()) + QString::number(maxSide) + "v2").toUtf8();
    const QString key = QString::fromLatin1(QCryptographicHash::hash(keyBytes, QCryptographicHash::Sha1).toHex());
    const QString outPath = QDir(cacheRoot()).filePath(key + suffix + ".png");
    if (QFileInfo::exists(outPath) && !QImage(outPath).isNull()) return outPath;
    QImage preview = image;
    if (qMax(image.width(), image.height()) > maxSide)
        preview = image.scaled(maxSide, maxSide, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QSaveFile file(outPath);
    if (!file.open(QIODevice::WriteOnly)) { *error = file.errorString(); return {}; }
    // PNG is built into Qt and preserves transparency; preview creation must not
    // depend on an optional JPEG writer plugin.
    QImageWriter writer(&file, "png");
    if (!writer.write(preview)) { *error = writer.errorString(); return {}; }
    if (!file.commit()) { *error = file.errorString(); return {}; }
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

void AppController::pushUndoSnapshot(ImageEntry &entry, const QVariantMap &snapshot) {
    if (snapshot == entry.settings) return;
    entry.undoStack.push_back(snapshot);
    if (entry.undoStack.size() > 100) entry.undoStack.removeFirst();
    entry.redoStack.clear();
}

void AppController::chooseImages(const QString &title, const QString &filter) {
    if (m_busy) return;
    // Pass QString paths directly, avoiding native QML URL-list conversions.
    const QStringList paths = QFileDialog::getOpenFileNames(nullptr, title, {}, filter);
    QVariantList values;
    for (const QString &path : paths) values.push_back(QUrl::fromLocalFile(path));
    importFiles(values);
}

void AppController::importFiles(const QVariantList &urls) {
    if (urls.isEmpty() || m_busy) return;
    m_importDetails.clear();
    emit importDetailsChanged();
    QStringList paths;
    QStringList failures;
    for (const QVariant &value : urls) {
        const QVariant v = value.metaType() == QMetaType::fromType<QJSValue>() ? value.value<QJSValue>().toVariant() : value;
        const QString text = v.toString();
        const QUrl url(text);
        // Test real paths first: a Windows drive letter must not be treated as a URL scheme.
        const QString path = QFileInfo(text).isFile() ? text : (url.isLocalFile() ? url.toLocalFile() : text);
        if (QFileInfo(path).isFile()) paths.push_back(QFileInfo(path).absoluteFilePath());
        else failures.push_back(text + "\n[path] File does not exist or is not a regular file");
    }
    if (paths.isEmpty()) {
        m_importDetails = failures.join("\n\n");
        emit importDetailsChanged();
        emit errorOccurred(QStringLiteral("error.noReadableImages"));
        return;
    }
    setBusy(true);
    setStatusText(QStringLiteral("status.preview"));
    m_workers.start([this, paths, failures]() mutable {
        QList<ImageEntry> decoded;
        for (const QString &path : paths) {
            QString reason;
            const QImage image = decodeImage(path, 2200, &reason);
            if (image.isNull()) { failures.push_back(path + "\n[decode] " + reason); continue; }
            ImageEntry e;
            e.originalPath = path;
            e.name = QFileInfo(path).fileName();
            e.previewPath = makePreview(path, image, 2200, "_preview", &reason);
            e.thumbPath = makePreview(path, image, 260, "_thumb", &reason);
            e.settings = defaultSettings();
            if (e.previewPath.isEmpty() || e.thumbPath.isEmpty()) {
                failures.push_back(path + "\n[cache] " + reason);
                continue;
            }
            decoded.push_back(e);
        }
        QMetaObject::invokeMethod(this, [this, decoded, failures]() {
            const int firstNew = m_images.size();
            m_images.append(decoded);
            emit imagesChanged();
            if (m_currentIndex < 0 && !decoded.isEmpty()) selectImage(firstNew);
            m_importDetails = failures.join("\n\n");
            emit importDetailsChanged();
            setStatusText(QStringLiteral("status.ready"));
            setBusy(false);
            if (decoded.isEmpty()) emit errorOccurred(QStringLiteral("error.noReadableImages"));
            else if (!failures.isEmpty()) emit errorOccurred(QStringLiteral("error.partialImport"));
        }, Qt::QueuedConnection);
    });
}

void AppController::selectImage(int index) {
    if (index < 0 || index >= m_images.size() || index == m_currentIndex) return;
    if (m_editInProgress) endSettingEdit();
    m_currentIndex = index;
    emit currentIndexChanged();
    emit currentImageChanged();
    emit currentSettingsChanged();
    emit historyChanged();
}

void AppController::beginSettingEdit() {
    if (m_currentIndex < 0 || m_currentIndex >= m_images.size() || m_editInProgress) return;
    m_editInProgress = true;
    m_editStartSettings = m_images.at(m_currentIndex).settings;
}

void AppController::setSetting(const QString &key, double value) {
    if (m_currentIndex < 0 || m_currentIndex >= m_images.size()) return;
    const QVariantMap defaults = defaultSettings();
    if (!defaults.contains(key) || !qIsFinite(value)) return;
    value = key == QStringLiteral("exposure") ? qBound(-3.0, value, 3.0) : qBound(-100.0, value, 100.0);
    if (!m_editInProgress) beginSettingEdit();
    m_images[m_currentIndex].settings.insert(key, value);
    emit currentSettingsChanged();
}

void AppController::endSettingEdit() {
    if (!m_editInProgress || m_currentIndex < 0 || m_currentIndex >= m_images.size()) return;
    ImageEntry &entry = m_images[m_currentIndex];
    const QVariantMap before = m_editStartSettings;
    m_editInProgress = false;
    m_editStartSettings.clear();
    pushUndoSnapshot(entry, before);
    emit historyChanged();
}

void AppController::resetCurrentSettings() {
    if (m_currentIndex < 0 || m_currentIndex >= m_images.size()) return;
    if (m_editInProgress) endSettingEdit();
    ImageEntry &entry = m_images[m_currentIndex];
    const QVariantMap before = entry.settings;
    const QVariantMap after = defaultSettings();
    if (before == after) return;
    entry.settings = after;
    pushUndoSnapshot(entry, before);
    emit currentSettingsChanged();
    emit historyChanged();
}

void AppController::copySettings() {
    if (m_currentIndex < 0 || m_currentIndex >= m_images.size()) return;
    m_copiedSettings = m_images.at(m_currentIndex).settings;
}

void AppController::pasteSettings() {
    if (m_currentIndex < 0 || m_currentIndex >= m_images.size() || m_copiedSettings.isEmpty()) return;
    ImageEntry &entry = m_images[m_currentIndex];
    if (entry.settings == m_copiedSettings) return;
    if (m_editInProgress) endSettingEdit();
    const QVariantMap before = entry.settings;
    entry.settings = m_copiedSettings;
    pushUndoSnapshot(entry, before);
    emit currentSettingsChanged();
    emit historyChanged();
}

void AppController::undo() {
    if (m_editInProgress) endSettingEdit();
    if (!canUndo()) return;
    ImageEntry &entry = m_images[m_currentIndex];
    entry.redoStack.push_back(entry.settings);
    entry.settings = entry.undoStack.takeLast();
    emit currentSettingsChanged();
    emit historyChanged();
}

void AppController::redo() {
    if (m_editInProgress) endSettingEdit();
    if (!canRedo()) return;
    ImageEntry &entry = m_images[m_currentIndex];
    entry.undoStack.push_back(entry.settings);
    entry.settings = entry.redoStack.takeLast();
    emit currentSettingsChanged();
    emit historyChanged();
}

void AppController::exportCurrent(const QUrl &folderUrl, const QString &format, int quality) {
    if (m_busy || m_currentIndex < 0 || m_currentIndex >= m_images.size()) return;
    if (m_editInProgress) endSettingEdit();
    const ImageEntry entry = m_images.at(m_currentIndex);
    const QVariantMap settings = entry.settings;
    const QString folder = folderUrl.isLocalFile() ? folderUrl.toLocalFile() : folderUrl.toString();
    if (folder.isEmpty()) { emit errorOccurred(QStringLiteral("error.noExportFolder")); return; }

    setBusy(true);
    setStatusText(QStringLiteral("status.export"));

    m_workers.start([this, entry, settings, folder, format, quality]() {
        QString decodeError;
        QImage original = decodeImage(entry.originalPath, 0, &decodeError);
        if (original.isNull()) {
            QMetaObject::invokeMethod(this, [this]() {
                setBusy(false); setStatusText(QStringLiteral("status.ready"));
                emit errorOccurred(QStringLiteral("error.readOriginal"));
            }, Qt::QueuedConnection);
            return;
        }

        QImage output = applyColorRecipe(original, settings);
        QDir().mkpath(folder);
        QString fmt = format.toLower();
        if (fmt != "png" && fmt != "webp") fmt = "jpg";
        const QString base = QFileInfo(entry.name).completeBaseName();
        QString outPath = QDir(folder).filePath(base + "_TBRetoch." + fmt);
        for (int copy = 1; QFileInfo::exists(outPath); ++copy)
            outPath = QDir(folder).filePath(base + "_TBRetoch_" + QString::number(copy) + "." + fmt);
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
            setStatusText(QStringLiteral("status.ready"));
            if (!ok) emit errorOccurred(error.isEmpty() ? QStringLiteral("error.exportFailed") : error);
            else emit exportFinished(outPath, bytes, width, height);
        }, Qt::QueuedConnection);
    });
}
