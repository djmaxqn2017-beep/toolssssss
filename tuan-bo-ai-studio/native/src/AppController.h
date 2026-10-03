#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QUrl>
#include <QThreadPool>
#include <QImage>

class AppController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList images READ images NOTIFY imagesChanged)
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(QString currentPreviewUrl READ currentPreviewUrl NOTIFY currentImageChanged)
    Q_PROPERTY(QString currentName READ currentName NOTIFY currentImageChanged)
    Q_PROPERTY(QVariantMap currentSettings READ currentSettings NOTIFY currentSettingsChanged)
    Q_PROPERTY(QString importDetails READ importDetails NOTIFY importDetailsChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY historyChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY historyChanged)

public:
    explicit AppController(QObject *parent = nullptr);
    ~AppController() override;

    QVariantList images() const;
    int currentIndex() const;
    QString currentPreviewUrl() const;
    QString currentName() const;
    QVariantMap currentSettings() const;
    bool busy() const;
    QString statusText() const;
    bool canUndo() const;
    bool canRedo() const;

    QString importDetails() const { return m_importDetails; }
    Q_INVOKABLE void chooseImages(const QString &title, const QString &filter);
    Q_INVOKABLE void importFiles(const QVariantList &urls);
    Q_INVOKABLE void selectImage(int index);
    Q_INVOKABLE void beginSettingEdit();
    Q_INVOKABLE void setSetting(const QString &key, double value);
    Q_INVOKABLE void endSettingEdit();
    Q_INVOKABLE void resetCurrentSettings();
    Q_INVOKABLE void copySettings();
    Q_INVOKABLE void pasteSettings();
    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();
    Q_INVOKABLE void exportCurrent(const QUrl &folderUrl, const QString &format, int quality);

signals:
    void imagesChanged();
    void importDetailsChanged();
    void currentIndexChanged();
    void currentImageChanged();
    void currentSettingsChanged();
    void busyChanged();
    void statusTextChanged();
    void historyChanged();
    void exportFinished(const QString &path, qint64 bytes, int width, int height);
    void errorOccurred(const QString &message);

private:
    struct ImageEntry {
        QString originalPath;
        QString name;
        QString previewPath;
        QString thumbPath;
        QVariantMap settings;
        QList<QVariantMap> undoStack;
        QList<QVariantMap> redoStack;
    };

    QThreadPool m_workers;
    QList<ImageEntry> m_images;
    int m_currentIndex = -1;
    bool m_busy = false;
    bool m_editInProgress = false;
    QVariantMap m_editStartSettings;
    QString m_importDetails;
    QString m_statusText = QStringLiteral("status.ready");
    QVariantMap m_copiedSettings;

    static QVariantMap defaultSettings();
    QString cacheRoot() const;
    QString makePreview(const QString &path, const QImage &image, int maxSide, const QString &suffix, QString *error) const;
    QVariantMap imageToVariant(const ImageEntry &entry, int index) const;
    void setBusy(bool value);
    void setStatusText(const QString &text);
    void pushUndoSnapshot(ImageEntry &entry, const QVariantMap &snapshot);
};
