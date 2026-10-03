#include <QtTest>
#include <QTemporaryDir>
#include <QDir>
#include <QImage>
#include <QImageReader>
#include <QSignalSpy>
#include <QFile>
#include <QJSEngine>
#include "ImageDecoder.h"
#include "AppController.h"

class ControllerTests : public QObject {
    Q_OBJECT
private slots:
    void codecAndUnicodeImport() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString folder = dir.filePath(QString::fromUtf8("Ảnh cưới # 100%"));
        QVERIFY(QDir().mkpath(folder));
        QImage image(120, 80, QImage::Format_ARGB32);
        image.fill(qRgba(60, 80, 100, 128));
        QVariantList urls;
        for (const QByteArray &format : {QByteArray("jpg"), QByteArray("png"), QByteArray("webp"), QByteArray("tiff")}) {
            const QString path = QDir(folder).filePath(QString::fromUtf8("Cô dâu 01.") + QString::fromLatin1(format));
            QVERIFY2(image.save(path, format.constData()), format.constData());
            urls.push_back(QUrl::fromLocalFile(path).toString(QUrl::FullyEncoded));
        }
        AppController c;
        QSignalSpy errors(&c, &AppController::errorOccurred);
        // Exercise the same JavaScript-array -> invokable boundary as QML.
        QJSEngine engine;
        engine.globalObject().setProperty("controller", engine.newQObject(&c));
        QJSValue selection = engine.newArray(urls.size());
        for (int i = 0; i < urls.size(); ++i) selection.setProperty(i, urls[i].toString());
        engine.globalObject().setProperty("selection", selection);
        const QJSValue result = engine.evaluate("controller.importFiles(selection)");
        QVERIFY2(!result.isError(), qPrintable(result.toString()));
        QTRY_VERIFY_WITH_TIMEOUT(!c.busy(), 30000);
        QVERIFY2(c.images().size() == 4, qPrintable(c.importDetails()));
        QCOMPARE(errors.count(), 0);
        QVERIFY(c.importDetails().isEmpty());
        c.selectImage(1);
        const QImage preview(QUrl(c.currentPreviewUrl()).toLocalFile());
        QCOMPARE(preview.pixelColor(10, 10).alpha(), 128);
    }

    void invalidAndMixedSelection() {
        QTemporaryDir dir;
        const QString valid = dir.filePath("valid.png");
        const QString invalid = dir.filePath("broken.jpg");
        QImage image(32, 32, QImage::Format_RGB32);
        image.fill(Qt::gray);
        QVERIFY(image.save(valid));
        QFile broken(invalid);
        QVERIFY(broken.open(QIODevice::WriteOnly));
        broken.write("not an image");
        broken.close();
        AppController c;
        QSignalSpy errors(&c, &AppController::errorOccurred);
        c.importFiles({QUrl::fromLocalFile(invalid), QUrl::fromLocalFile(valid), dir.filePath("missing.png")});
        QTRY_VERIFY_WITH_TIMEOUT(!c.busy(), 30000);
        QCOMPARE(c.images().size(), 1);
        QCOMPARE(c.currentIndex(), 0);
        QCOMPARE(errors.count(), 1);
        QCOMPARE(errors.first().first().toString(), QString("error.partialImport"));
        QVERIFY(c.importDetails().contains("[decode]"));
        QVERIFY(c.importDetails().contains("[path]"));
    }

    void heicDecodeAndExport() {
        const QString fixture = qEnvironmentVariable("TBRETOCH_HEIC_FIXTURE");
        if (fixture.isEmpty()) QSKIP("Set TBRETOCH_HEIC_FIXTURE to the upstream HEIC fixture");
        QTemporaryDir dir;
        const QString path = dir.filePath(QString::fromUtf8("Cô dâu.heic"));
        QVERIFY(QFile::copy(fixture, path));
        QString error;
        const QImage original = decodeImage(path, 0, &error);
        QVERIFY2(!original.isNull(), qPrintable(error));
        AppController c;
        c.importFiles({QUrl::fromLocalFile(path)});
        QTRY_VERIFY_WITH_TIMEOUT(!c.busy(), 30000);
        QVERIFY2(c.images().size() == 1, qPrintable(c.importDetails()));
        QSignalSpy exported(&c, &AppController::exportFinished);
        c.exportCurrent(QUrl::fromLocalFile(dir.path()), "png", 98);
        QTRY_COMPARE_WITH_TIMEOUT(exported.count(), 1, 30000);
        const QImage output(exported.first()[0].toString());
        QCOMPARE(output.size(), original.size());
    }

    void rawDecodeAndExport() {
        QTemporaryDir dir;
        const QString path = dir.filePath(QString::fromUtf8("Ảnh gốc.dng"));
        QVERIFY(QFile::copy(QStringLiteral(":/fixtures/sample.dng"), path));
        AppController c;
        c.importFiles({path});
        QTRY_VERIFY_WITH_TIMEOUT(!c.busy(), 30000);
        QVERIFY2(c.images().size() == 1, qPrintable(c.importDetails()));
        QSignalSpy exported(&c, &AppController::exportFinished);
        c.exportCurrent(QUrl::fromLocalFile(dir.path()), "png", 98);
        QTRY_COMPARE_WITH_TIMEOUT(exported.count(), 1, 30000);
        const QImage output(exported.first()[0].toString());
        QVERIFY(!output.isNull());
        QVERIFY(output.width() >= 100);
        QVERIFY(output.height() >= 80);
    }

    void imageWorkflow() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QImage source(6000, 4000, QImage::Format_RGB32);
        source.fill(QColor(60, 80, 100));
        const QString path = dir.filePath("source.png");
        QVERIFY(source.save(path));
        AppController c;
        QSignalSpy errors(&c, &AppController::errorOccurred);
        c.importFiles({QUrl::fromLocalFile(path), QUrl::fromLocalFile(path)});
        QVERIFY(c.busy());
        QTRY_VERIFY_WITH_TIMEOUT(!c.busy(), 30000);
        QCOMPARE(errors.count(), 0);
        QCOMPARE(c.images().size(), 2);
        QCOMPARE(c.currentIndex(), 0);
        QImageReader preview(QUrl(c.currentPreviewUrl()).toLocalFile());
        QVERIFY(preview.size().width() <= 2200);

        c.beginSettingEdit();
        c.setSetting("exposure", 0.5);
        c.setSetting("exposure", 1.0);
        c.endSettingEdit();
        QVERIFY(c.canUndo());
        c.undo();
        QCOMPARE(c.currentSettings().value("exposure").toDouble(), 0.0);
        QVERIFY(!c.canUndo());
        c.redo();
        QCOMPARE(c.currentSettings().value("exposure").toDouble(), 1.0);

        c.resetCurrentSettings();
        QCOMPARE(c.currentSettings().value("exposure").toDouble(), 0.0);
        c.undo();
        QCOMPARE(c.currentSettings().value("exposure").toDouble(), 1.0);
        c.copySettings();
        c.selectImage(1);
        c.pasteSettings();
        QCOMPARE(c.currentSettings().value("exposure").toDouble(), 1.0);
        c.undo();
        QCOMPARE(c.currentSettings().value("exposure").toDouble(), 0.0);
        c.redo();

        QSignalSpy exported(&c, &AppController::exportFinished);
        c.exportCurrent(QUrl::fromLocalFile(dir.path()), "png", 98);
        QVERIFY(c.busy());
        QTRY_COMPARE_WITH_TIMEOUT(exported.count(), 1, 60000);
        QCOMPARE(errors.count(), 0);
        const QList<QVariant> result = exported.takeFirst();
        QCOMPARE(result[2].toInt(), 6000);
        QCOMPARE(result[3].toInt(), 4000);
        const QImage output(result[0].toString());
        QCOMPARE(output.size(), source.size());
        const QColor px = output.pixelColor(100, 100);
        QVERIFY(qAbs(px.red() - 120) <= 1);
        QVERIFY(qAbs(px.green() - 160) <= 1);
        QVERIFY(qAbs(px.blue() - 200) <= 1);
        QCOMPARE(source.pixelColor(100, 100), QColor(60, 80, 100));
    }

    void selectionCommitsPendingEdit() {
        QTemporaryDir dir;
        QImage img(80, 60, QImage::Format_RGB32);
        img.fill(Qt::gray);
        QVERIFY(img.save(dir.filePath("test.png")));
        AppController c;
        c.importFiles({QUrl::fromLocalFile(dir.filePath("test.png")), QUrl::fromLocalFile(dir.filePath("test.png"))});
        QTRY_VERIFY(!c.busy());
        c.setSetting("contrast", 23);
        c.selectImage(1);
        c.selectImage(0);
        QVERIFY(c.canUndo());
        c.undo();
        QCOMPARE(c.currentSettings().value("contrast").toDouble(), 0.0);
        c.setSetting("exposure", 500);
        c.endSettingEdit();
        QCOMPARE(c.currentSettings().value("exposure").toDouble(), 3.0);
        c.setSetting("unknown", 30);
        QVERIFY(!c.currentSettings().contains("unknown"));
    }
};

QTEST_MAIN(ControllerTests)
#include "ControllerTests.moc"
