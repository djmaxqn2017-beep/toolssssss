#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QUrl>

class AppController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList images READ images NOTIFY imagesChanged)
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(QString currentPreviewUrl READ currentPreviewUrl NOTIFY currentImageChanged)
    Q_PROPERTY(QString currentName READ currentName NOTIFY currentImageChanged)
    Q_PROPERTY(QVariantMap currentSettings READ currentSettings NOTIFY currentSettingsChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)

public:
    explicit AppController(QObject *parent = nullptr);

    QVariantList images() const;
    int currentIndex() const;
    QString currentPreviewUrl() const;
    QString currentName() const;
    QVariantMap currentSettings() const;
    bool busy() const;
    QString statusText() const;

    Q_INVOKABLE void importFiles(const QVariantList &urls);
    Q_INVOKABLE void selectImage(int index);
    Q_INVOKABLE void setSetting(const QString &key, double value);
    Q_INVOKABLE void resetCurrentSettings();
    Q_INVOKABLE void copySettings();
    Q_INVOKABLE void pasteSettings();
    Q_INVOKABLE void exportCurrent(const QUrl &folderUrl, const QString &format, int quality);

signals:
    void imagesChanged();
    void currentIndexChanged();
    void currentImageChanged();
    void currentSettingsChanged();
    void busyChanged();
    void statusTextChanged();
    void exportFinished(const QString &path, qint64 bytes, int width, int height);
    void errorOccurred(const QString &message);

private:
    struct ImageEntry {
        QString originalPath;
        QString name;
        QString previewPath;
        QString thumbPath;
        QVariantMap settings;
    };

    QList<ImageEntry> m_images;
    int m_currentIndex = -1;
    bool m_busy = false;
    QString m_statusText = QStringLiteral("GPU Preview • CPU Export • Offline");
    QVariantMap m_copiedSettings;

    static QVariantMap defaultSettings();
    QString cacheRoot() const;
    QString makePreview(const QString &path, int maxSide, const QString &suffix) const;
    QVariantMap imageToVariant(const ImageEntry &entry, int index) const;
    void setBusy(bool value);
    void setStatusText(const QString &text);
};
