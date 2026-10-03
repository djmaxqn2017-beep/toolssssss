#include "AppController.h"
#include "ImageDecoder.h"
#include "AdvancedRecipe.h"
#include "SemanticEngine.h"
#include "ExportMetadata.h"
#include <QColorSpace>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
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
static int clamp16(double v) { return qBound(0, qRound(v), 65535); }
static double clamp01(double v) { return qBound(0.0, v, 1.0); }

static QImage applyColorRecipe(QImage img, const QVariantMap &s) {
    bool changed = false;
    for (const char *key : {"exposure","contrast","highlights","shadows","whites","blacks","temperature","tint","saturation","vibrance","clarity","dehaze","fade"}) changed |= s.value(key).toDouble() != 0;
    if (!changed) return img;
    img = img.convertToFormat(QImage::Format_RGBA64);
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
        auto *line = reinterpret_cast<QRgba64*>(img.scanLine(y));
        for (int x = 0; x < img.width(); ++x) {
            const QRgba64 px = line[x];
            double r = px.red() / 65535.0;
            double g = px.green() / 65535.0;
            double b = px.blue() / 65535.0;

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

            line[x] = QRgba64::fromRgba64(clamp16(r*65535.0), clamp16(g*65535.0), clamp16(b*65535.0), px.alpha());
        }
    }
    return img;
}
}

AppController::AppController(QObject *parent) : QObject(parent) {
    QImageReader::setAllocationLimit(1024);
    m_workers.setMaxThreadCount(2);
    m_previewTimer.setSingleShot(true);
    m_previewTimer.setInterval(45);
    connect(&m_previewTimer, &QTimer::timeout, this, &AppController::renderPreview);
    connect(this, &AppController::currentSettingsChanged, this, &AppController::schedulePreview);
    connect(this, &AppController::currentSettingsChanged, this, &AppController::analysisChanged);
}
AppController::~AppController() { m_workers.waitForDone(); }

QVariantMap AppController::defaultSettings() {
    QVariantMap settings = {
        {"exposure", 0.0}, {"contrast", 0.0}, {"highlights", 0.0}, {"shadows", 0.0},
        {"whites", 0.0}, {"blacks", 0.0}, {"temperature", 0.0}, {"tint", 0.0},
        {"saturation", 0.0}, {"vibrance", 0.0}, {"clarity", 0.0}, {"dehaze", 0.0},
        {"fade", 0.0}
    };
    const QVariantMap extra = advancedDefaults();
    for (auto it = extra.cbegin(); it != extra.cend(); ++it) settings.insert(it.key(), it.value());
    const auto semantic = semanticDefaults();
    for (auto it = semantic.cbegin(); it != semantic.cend(); ++it) settings.insert(it.key(),it.value());
    return settings;
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
        {"originalPath", entry.originalPath}, {"selected", entry.selected}
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
    m_selectedFace=-1;
    emit analysisChanged();
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
    if (!defaults.contains(canonicalSettingKey(key)) || !qIsFinite(value) || canonicalSettingKey(key).endsWith("File")) return;
    if (key == "exposure" || key.endsWith("_exposure")) value = qBound(-3.0, value, 3.0);
    else if (key.startsWith("crop")) value = qBound(0.0, value, 45.0);
    else if (key == "rotation") value = qBound(0, qRound(value), 3);
    else if (key == "straighten") value = qBound(-45.0, value, 45.0);
    else if (key.startsWith("flip")) value = value >= .5 ? 1 : 0;
    else if (key == "sharpness" || key == "denoise" || key == "grain") value = qBound(0.0, value, 100.0);
    else value = qBound(-100.0, value, 100.0);
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

QString AppController::renderedPreviewUrl() const {
    if (m_currentIndex < 0 || m_currentIndex >= m_images.size()) return {};
    const ImageEntry &entry = m_images.at(m_currentIndex);
    return QUrl::fromLocalFile(entry.renderedPath.isEmpty() ? entry.previewPath : entry.renderedPath).toString();
}

bool AppController::useRenderedPreview() const { return m_currentIndex >= 0; }

void AppController::schedulePreview() {
    ++m_previewGeneration;
    m_previewTimer.stop();
    if (!useRenderedPreview()) { emit previewChanged(); return; }
    m_images[m_currentIndex].renderedPath.clear();
    m_previewTimer.start();
    emit previewChanged();
}

void AppController::renderPreview() {
    if (m_previewActive || !useRenderedPreview() || m_currentIndex < 0) return;
    const int index = m_currentIndex, generation = m_previewGeneration;
    const ImageEntry entry = m_images.at(index);
    const bool needSky = m_activeMask == "sky";
    m_previewActive = true;
    emit analysisChanged();
    emit previewChanged();
    m_workers.start([this, entry, index, generation, needSky]() {
        const QByteArray signature = QByteArray("pipeline-v7.0.0") + entry.previewPath.toUtf8() + QJsonDocument::fromVariant(entry.settings).toJson(QJsonDocument::Compact);
        const QString path = QDir(cacheRoot()).filePath(QString::fromLatin1(QCryptographicHash::hash(signature,QCryptographicHash::Sha256).toHex()) + "_edited.png");
        bool ok = !QImage(path).isNull();
        QString error;
        auto analysis = entry.analysis;
        try {
            QImage image(entry.previewPath);
            if(qMax(image.width(),image.height())>1200)image=image.scaled(1200,1200,Qt::KeepAspectRatio,Qt::SmoothTransformation);
            if (hasSemanticSettings(entry.settings) || m_analyseRequested.load()) {
                analysis = analysePortrait(image,entry.previewPath+"_semantic",entry.settings.value("lensBlur").toDouble()!=0,needSky || entry.settings.value("skyReplacement").toDouble()!=0 || entry.settings.value("mask_sky_exposure").toDouble()!=0 || entry.settings.value("mask_sky_contrast").toDouble()!=0 || entry.settings.value("mask_sky_saturation").toDouble()!=0 || entry.settings.value("mask_sky_temperature").toDouble()!=0);
            }
            if (!ok) {
                image = applyColorRecipe(image,entry.settings);
                if (hasSemanticSettings(entry.settings) && analysis) image = applyPortraitRecipe(image,entry.settings,*analysis);
                image = applyAdvancedRecipe(image,entry.settings);
                QSaveFile file(path);
                if (!image.isNull() && file.open(QIODevice::WriteOnly)) {
                    QImageWriter writer(&file,"png");
                    ok = writer.write(image) && file.commit();
                    if (!ok) error = writer.errorString();
                } else error = file.errorString();
            }
        } catch (const std::exception &e) { ok=false;error=QString::fromUtf8(e.what()); }
        QMetaObject::invokeMethod(this,[this,index,generation,path,ok,error,analysis]() {
            m_previewActive = false;
            if (index < m_images.size() && analysis) { m_images[index].analysis = analysis; emit analysisChanged(); }
            if (generation == m_previewGeneration && index == m_currentIndex) {
                if (ok) m_images[index].renderedPath = path;
                else emit errorOccurred(error.isEmpty() ? QStringLiteral("error.previewFailed") : error);
                emit previewChanged();
            }
            if (generation != m_previewGeneration && useRenderedPreview()) m_previewTimer.start();
        },Qt::QueuedConnection);
    });
}

void AppController::setSelected(int index, bool selected) {
    if (index < 0 || index >= m_images.size()) return;
    m_images[index].selected = selected;
    emit imagesChanged();
}
void AppController::selectAll(bool selected) {
    for (ImageEntry &entry : m_images) entry.selected = selected;
    emit imagesChanged();
}
void AppController::syncSelected(const QString &group) {
    if (m_currentIndex < 0 || (group != "all" && group != "color" && group != "geometry" && group != "detail")) return;
    if (m_editInProgress) endSettingEdit();
    const QVariantMap source = currentSettings();
    for (int i = 0; i < m_images.size(); ++i) {
        if (i == m_currentIndex || !m_images[i].selected) continue;
        ImageEntry &entry = m_images[i];
        const QVariantMap before = entry.settings;
        for (auto it = source.cbegin(); it != source.cend(); ++it) {
            const bool geometry = isGeometryKey(it.key());
            const bool detail = it.key() == "sharpness" || it.key() == "denoise";
            const bool semantic = isSemanticKey(it.key());
            if (group == "all" || (group == "geometry" && geometry) || (group == "detail" && detail) || (group == "color" && !geometry && !detail && !semantic)) entry.settings.insert(it.key(),it.value());
        }
        pushUndoSnapshot(entry,before);
        entry.renderedPath.clear();
    }
    emit imagesChanged();
    emit historyChanged();
}
void AppController::savePreset(const QUrl &fileUrl) {
    if (m_currentIndex < 0 || !fileUrl.isLocalFile()) return;
    if (m_editInProgress) endSettingEdit();
    QSaveFile file(fileUrl.toLocalFile());
    if (!file.open(QIODevice::WriteOnly)) { emit errorOccurred(file.errorString()); return; }
    const QByteArray data = QJsonDocument(QJsonObject{{"schema",1},{"settings",QJsonObject::fromVariantMap(currentSettings())}}).toJson();
    if (file.write(data) != data.size() || !file.commit()) emit errorOccurred(file.errorString());
}
void AppController::loadPreset(const QUrl &fileUrl) {
    if (m_currentIndex < 0 || !fileUrl.isLocalFile()) return;
    QFile file(fileUrl.toLocalFile());
    if (!file.open(QIODevice::ReadOnly)) { emit errorOccurred(file.errorString()); return; }
    QJsonParseError parse;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(),&parse);
    const QJsonObject object = document.object();
    if (parse.error != QJsonParseError::NoError || object.value("schema").toInt() != 1 || !object.value("settings").isObject()) {
        emit errorOccurred(QStringLiteral("error.invalidPreset")); return;
    }
    const QVariantMap values = object.value("settings").toObject().toVariantMap();
    const QVariantMap defaults = defaultSettings();
    for (auto it = values.cbegin(); it != values.cend(); ++it) {
        bool numeric = false;
        const double value = it.value().toDouble(&numeric);
        const QString base=canonicalSettingKey(it.key());
        if (base.endsWith("File") && defaults.contains(base) && it.value().metaType().id()==QMetaType::QString) continue;
        if (!defaults.contains(base) || !numeric || !qIsFinite(value)) { emit errorOccurred(QStringLiteral("error.invalidPreset")); return; }
    }
    if (m_editInProgress) endSettingEdit();
    beginSettingEdit();
    // Missing keys reset to defaults, so older presets do not retain hidden effects.
    m_images[m_currentIndex].settings = defaults;
    for (auto it = values.cbegin(); it != values.cend(); ++it) {
        if (it.key().endsWith("File")) m_images[m_currentIndex].settings.insert(it.key(),it.value());
        else setSetting(it.key(),it.value().toDouble());
    }
    emit currentSettingsChanged();
    endSettingEdit();
}

void AppController::exportCurrent(const QUrl &folderUrl, const QString &format, int quality) {
    if (m_busy || m_currentIndex < 0 || m_currentIndex >= m_images.size()) return;
    if (m_editInProgress) endSettingEdit();
    exportEntries({m_images.at(m_currentIndex)},folderUrl,format,quality);
}
void AppController::exportSelected(const QUrl &folderUrl, const QString &format, int quality) {
    if (m_busy) return;
    if (m_editInProgress) endSettingEdit();
    QList<ImageEntry> entries;
    for (const ImageEntry &entry : m_images) if (entry.selected) entries.push_back(entry);
    if (entries.isEmpty()) { emit errorOccurred(QStringLiteral("error.noSelection")); return; }
    exportEntries(entries,folderUrl,format,quality);
}
void AppController::cancelExport() { m_cancelExport.store(true); }

void AppController::exportEntries(const QList<ImageEntry> &entries, const QUrl &folderUrl, const QString &format, int quality) {
    if (entries.isEmpty() || m_busy) return;
    const QString folder = folderUrl.isLocalFile() ? folderUrl.toLocalFile() : folderUrl.toString();
    if (folder.isEmpty()) { emit errorOccurred(QStringLiteral("error.noExportFolder")); return; }
    m_cancelExport.store(false);
    m_exportCompleted = 0; m_exportTotal = entries.size();
    emit exportProgressChanged();
    setBusy(true); setStatusText(QStringLiteral("status.export"));
    m_workers.start([this,entries,folder,format,quality]() {
        int succeeded = 0, failed = 0, completed = 0;
        const bool folderOk = QDir().mkpath(folder);
        for (const ImageEntry &entry : entries) {
            if (m_cancelExport.load()) break;
            QString error;
            QImage original = folderOk ? decodeImage(entry.originalPath,0,&error) : QImage();
            bool ok = !original.isNull();
            QString outPath;
            qint64 bytes = 0;
            int width = 0, height = 0;
            if (ok) {
                try {
                    bool edited=false;
                    for(auto it=entry.settings.cbegin();it!=entry.settings.cend();++it) if(!it.key().endsWith("File") && it.value().toDouble()!=0) edited=true;
                    QString fmt = format.toLower();
                    const bool exact = (fmt == "master" || fmt == "original") && !edited;
                    if (exact) fmt = QFileInfo(entry.originalPath).suffix().toLower();
                    else if (fmt == "master" || fmt == "original") fmt = "png";
                    else if (fmt != "png" && fmt != "webp" && fmt != "tiff") fmt = "jpg";
                    const QString base = QFileInfo(entry.name).completeBaseName();
                    outPath = QDir(folder).filePath(base+"_TBRetoch."+fmt);
                    for (int copy = 1; QFileInfo::exists(outPath); ++copy) outPath = QDir(folder).filePath(base+"_TBRetoch_"+QString::number(copy)+"."+fmt);
                    width=original.width();height=original.height();
                    if (exact) {
                        QFile source(entry.originalPath);QSaveFile target(outPath);ok=source.open(QIODevice::ReadOnly)&&target.open(QIODevice::WriteOnly);
                        while(ok&&!source.atEnd()){auto block=source.read(1024*1024);if(block.isEmpty()&&source.error()!=QFile::NoError){ok=false;break;}ok=target.write(block)==block.size();}
                        ok=ok&&target.commit();if(!ok)error=source.errorString()+" "+target.errorString();
                    } else {
                        QImage output=applyColorRecipe(original,entry.settings);
                        if(hasSemanticSettings(entry.settings)) {
                            auto analysis=analysePortrait(original,entry.previewPath+"_semantic",entry.settings.value("lensBlur").toDouble()!=0,entry.settings.value("skyReplacement").toDouble()!=0||entry.settings.value("mask_sky_exposure").toDouble()!=0||entry.settings.value("mask_sky_contrast").toDouble()!=0||entry.settings.value("mask_sky_saturation").toDouble()!=0||entry.settings.value("mask_sky_temperature").toDouble()!=0);
                            output=applyPortraitRecipe(output,entry.settings,*analysis);
                        }
                        output=applyAdvancedRecipe(output,entry.settings);
                        if(fmt=="png"||fmt=="tiff")output=output.convertToFormat(QImage::Format_RGBA64);
                        QTemporaryFile encoded(QDir(folder).filePath(".TBRetoch-XXXXXX."+fmt));
                        ok=encoded.open();QString temporary=encoded.fileName();encoded.close();
                        if(ok){QImageWriter writer(temporary,fmt=="jpg"?QByteArray("jpeg"):fmt.toLatin1());if(fmt=="jpg"||fmt=="webp")writer.setQuality(qBound(0,quality,100));
                            if(fmt=="png")writer.setCompression(70);
                            if(fmt=="tiff")writer.setCompression(1);ok=writer.write(output);if(!ok)error=writer.errorString();}
                        if(ok)ok=preserveExportMetadata(entry.originalPath,temporary,output.size(),&error);
                        if(ok){QFile source(temporary);QSaveFile target(outPath);ok=source.open(QIODevice::ReadOnly)&&target.open(QIODevice::WriteOnly);while(ok&&!source.atEnd()){auto block=source.read(1024*1024);ok=!block.isEmpty()&&target.write(block)==block.size();}ok=ok&&target.commit();if(!ok)error=target.errorString();}
                        width=output.width();height=output.height();
                    }
                    bytes=ok?QFileInfo(outPath).size():0;
                } catch(const std::exception &e) {ok=false;error=QString::fromUtf8(e.what());}

            }
            if (ok) ++succeeded; else ++failed;
            ++completed;
            QMetaObject::invokeMethod(this,[this,ok,outPath,bytes,width,height,error,completed]() {
                m_exportCompleted = completed; emit exportProgressChanged();
                if (ok) emit exportFinished(outPath,bytes,width,height);
                else emit errorOccurred(error.isEmpty() ? QStringLiteral("error.exportFailed") : error);
            },Qt::QueuedConnection);
        }
        const bool cancelled = m_cancelExport.load();
        QMetaObject::invokeMethod(this,[this,succeeded,failed,cancelled]() {
            setBusy(false); setStatusText(QStringLiteral("status.ready"));
            emit exportQueueFinished(succeeded,failed,cancelled);
        },Qt::QueuedConnection);
    });
}

int AppController::faceCount() const {return m_currentIndex>=0&&m_images[m_currentIndex].analysis?m_images[m_currentIndex].analysis->faces.size():0;}
QString AppController::analysisStatus() const {
    if(m_previewActive&&m_analyseRequested.load())return QStringLiteral("Đang phân tích ảnh…");
    if(m_currentIndex<0||!m_images[m_currentIndex].analysis)return QStringLiteral("Mở ảnh để phân tích vùng chỉnh sửa");
    return QStringLiteral("%1 khuôn mặt · %2").arg(faceCount()).arg(m_images[m_currentIndex].analysis->backend);
}
void AppController::analyseCurrent() {m_analyseRequested.store(true);schedulePreview();emit analysisChanged();}
void AppController::selectMask(const QString &name) {
    if(!semanticMaskNames().contains(name)||m_currentIndex<0)return;m_activeMask=name;analyseCurrent();emit analysisChanged();
}
QString AppController::maskPreviewUrl() const {
    if(m_currentIndex<0||m_activeMask.isEmpty()||!m_images[m_currentIndex].analysis)return {};
    const auto &entry=m_images[m_currentIndex];
    if(!entry.analysis->masks.contains(m_activeMask))return {};
    const bool facialMask = m_activeMask=="faceSkin" || m_activeMask=="eyes" || m_activeMask=="lips" || m_activeMask=="teeth";
    QImage mask=semanticMask(*entry.analysis,m_activeMask,QImage(entry.previewPath).size(),facialMask?m_selectedFace:-1);
    QImage overlay(mask.size(),QImage::Format_RGBA8888);
    for(int y=0;y<mask.height();++y){const auto *src=mask.constScanLine(y);auto *dst=overlay.scanLine(y);for(int x=0;x<mask.width();++x){dst[x*4]=214;dst[x*4+1]=96;dst[x*4+2]=255;dst[x*4+3]=src[x]/2;}}
    QVariantMap geometry=advancedDefaults();for(auto it=geometry.begin();it!=geometry.end();++it)if(isGeometryKey(it.key()))it.value()=entry.settings.value(it.key());
    overlay=applyAdvancedRecipe(overlay,geometry);
    const auto hash=QCryptographicHash::hash(QJsonDocument::fromVariant(geometry).toJson(),QCryptographicHash::Sha1).toHex();
    QString path=entry.previewPath+"_semantic/overlay_"+m_activeMask+"_"+QString::number(m_selectedFace)+"_"+hash+".png";
    if(!QFileInfo::exists(path)&&!overlay.save(path))return {};
    return QUrl::fromLocalFile(path).toString();
}
void AppController::setSelectedFace(int face) {m_selectedFace=qBound(-1,face,faceCount()-1);emit analysisChanged();}
void AppController::chooseReplacement(bool sky) {
    if(m_currentIndex<0)return;QString file=QFileDialog::getOpenFileName(nullptr,sky?QStringLiteral("Chọn ảnh bầu trời"):QStringLiteral("Chọn ảnh nền"),{},QStringLiteral("Images (*.jpg *.jpeg *.png *.tif *.tiff *.webp)"));if(file.isEmpty())return;
    if(QImage(file).isNull()){emit errorOccurred(QStringLiteral("Không đọc được ảnh thay thế"));return;}
    beginSettingEdit();m_images[m_currentIndex].settings.insert(sky?"skyFile":"backgroundFile",file);setSetting(sky?"skyReplacement":"backgroundReplacement",100);endSettingEdit();
}
