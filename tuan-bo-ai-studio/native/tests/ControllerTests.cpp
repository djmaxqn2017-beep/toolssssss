#include <QtTest>
#include <QTemporaryDir>
#include <QImage>
#include <QImageReader>
#include <QSignalSpy>
#include "AppController.h"

class ControllerTests : public QObject {
    Q_OBJECT
private slots:
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
