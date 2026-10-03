#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QUrl>
#include <QThreadPool>
#include <QImage>
#include <QTimer>
#include <atomic>
#include "SemanticEngine.h"

class AppController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(int faceCount READ faceCount NOTIFY analysisChanged)
    Q_PROPERTY(int selectedFace READ selectedFace WRITE setSelectedFace NOTIFY analysisChanged)
    Q_PROPERTY(QString analysisStatus READ analysisStatus NOTIFY analysisChanged)
    Q_PROPERTY(QString activeMask READ activeMask NOTIFY analysisChanged)
    Q_PROPERTY(QString maskPreviewUrl READ maskPreviewUrl NOTIFY analysisChanged)
    Q_PROPERTY(QVariantList images READ images NOTIFY imagesChanged)
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(QString currentPreviewUrl READ currentPreviewUrl NOTIFY currentImageChanged)
    Q_PROPERTY(QString renderedPreviewUrl READ renderedPreviewUrl NOTIFY previewChanged)
    Q_PROPERTY(bool useRenderedPreview READ useRenderedPreview NOTIFY currentSettingsChanged)
    Q_PROPERTY(bool previewBusy READ previewBusy NOTIFY previewChanged)
    Q_PROPERTY(int exportCompleted READ exportCompleted NOTIFY exportProgressChanged)
    Q_PROPERTY(int exportTotal READ exportTotal NOTIFY exportProgressChanged)
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
    QString renderedPreviewUrl() const;
    bool useRenderedPreview() const;
    bool previewBusy() const { return m_previewActive || m_previewTimer.isActive(); }
    int exportCompleted() const { return m_exportCompleted; }
    int exportTotal() const { return m_exportTotal; }
    QVariantMap currentSettings() const;
    bool busy() const;
    QString statusText() const;
    bool canUndo() const;
    bool canRedo() const;

    int faceCount() const;
    int selectedFace() const { return m_selectedFace; }
    QString analysisStatus() const;
    QString activeMask() const { return m_activeMask; }
    QString maskPreviewUrl() const;
    Q_INVOKABLE void analyseCurrent();
    Q_INVOKABLE void selectMask(const QString &name);
    Q_INVOKABLE void setSelectedFace(int face);
    Q_INVOKABLE void chooseReplacement(bool sky);
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
    Q_INVOKABLE void setSelected(int index, bool selected);
    Q_INVOKABLE void selectAll(bool selected);
    Q_INVOKABLE void syncSelected(const QString &group);
    Q_INVOKABLE void savePreset(const QUrl &fileUrl);
    Q_INVOKABLE void loadPreset(const QUrl &fileUrl);
    Q_INVOKABLE void exportSelected(const QUrl &folderUrl, const QString &format, int quality);
    Q_INVOKABLE void cancelExport();
    Q_INVOKABLE void exportCurrent(const QUrl &folderUrl, const QString &format, int quality);

signals:
    void analysisChanged();
    void imagesChanged();
    void previewChanged();
    void exportProgressChanged();
    void exportQueueFinished(int succeeded, int failed, bool cancelled);
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
        QString renderedPath;
        bool selected = true;
        QVariantMap settings;
        std::shared_ptr<PortraitAnalysis> analysis;
        QList<QVariantMap> undoStack;
        QList<QVariantMap> redoStack;
    };

    std::atomic_bool m_analyseRequested{false};
    QString m_activeMask;
    int m_selectedFace = -1;
    QThreadPool m_workers;
    QTimer m_previewTimer;
    int m_previewGeneration = 0;
    bool m_previewActive = false;
    int m_exportCompleted = 0;
    int m_exportTotal = 0;
    std::atomic_bool m_cancelExport{false};
    QList<ImageEntry> m_images;
    int m_currentIndex = -1;
    bool m_busy = false;
    bool m_editInProgress = false;
    QVariantMap m_editStartSettings;
    QString m_importDetails;
    QString m_statusText = QStringLiteral("status.ready");
    QVariantMap m_copiedSettings;

    static QVariantMap defaultSettings();
    void schedulePreview();
    void renderPreview();
    void exportEntries(const QList<ImageEntry> &entries, const QUrl &folderUrl, const QString &format, int quality);
    QString cacheRoot() const;
    QString makePreview(const QString &path, const QImage &image, int maxSide, const QString &suffix, QString *error) const;
    QVariantMap imageToVariant(const ImageEntry &entry, int index) const;
    void setBusy(bool value);
    void setStatusText(const QString &text);
    void pushUndoSnapshot(ImageEntry &entry, const QVariantMap &snapshot);
};
