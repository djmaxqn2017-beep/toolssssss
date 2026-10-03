#include <QApplication>
#include <QDir>
#include <QTimer>
#include <QFile>
#include <QDebug>
#include <QQuickStyle>
#include <QQuickItem>
#include <QTemporaryDir>
#include <QImage>
#include <QImageReader>
#include <QImageWriter>
#include <memory>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QSGRendererInterface>

#include "AppController.h"
#include "LocalizationManager.h"

int main(int argc, char *argv[]) {
#if defined(Q_OS_WIN)
    QQuickWindow::setGraphicsApi(QSGRendererInterface::Direct3D11);
#endif
    QQuickStyle::setStyle(QStringLiteral("Fusion"));
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("TB"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("tbretouch.local"));
    QCoreApplication::setApplicationName(QStringLiteral("TBRetoch"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.6.2-workflow"));

    AppController controller;
    LocalizationManager i18n;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("appController"), &controller);
    engine.rootContext()->setContextProperty(QStringLiteral("i18n"), &i18n);

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app, []() {
        QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);

    engine.loadFromModule("TBRetoch", "Main");
    std::unique_ptr<QTemporaryDir> renderFixture;
    if (app.arguments().contains(QStringLiteral("--import-smoke-test"))) {
        renderFixture = std::make_unique<QTemporaryDir>();
        const QString folder = renderFixture->filePath(QString::fromUtf8("Ảnh cưới # 100%"));
        if (!QDir().mkpath(folder)) return 10;
        QImage image(400, 300, QImage::Format_ARGB32);
        image.fill(qRgba(60, 80, 100, 128));
        QVariantList urls;
        for (const QByteArray &format : {QByteArray("jpg"), QByteArray("png"), QByteArray("webp"), QByteArray("tiff")}) {
            const QString path = QDir(folder).filePath(QString::fromUtf8("Cô dâu.") + QString::fromLatin1(format));
            if (!image.save(path, format.constData())) return 11;
            urls.push_back(QUrl::fromLocalFile(path).toString(QUrl::FullyEncoded));
        }
        const QString rawPath = QDir(folder).filePath(QString::fromUtf8("Ảnh gốc.dng"));
        if (!QFile::copy(QStringLiteral(":/fixtures/sample.dng"), rawPath)) return 12;
        urls.push_back(QUrl::fromLocalFile(rawPath).toString(QUrl::FullyEncoded));
        const QString heicPath = QDir(folder).filePath(QString::fromUtf8("Cô dâu.heic"));
        if (!QFile::copy(qEnvironmentVariable("TBRETOCH_HEIC_FIXTURE"), heicPath)) return 13;
        urls.push_back(QUrl::fromLocalFile(heicPath).toString(QUrl::FullyEncoded));
        QObject::connect(&controller, &AppController::busyChanged, &app, [&]() {
            if (controller.busy()) return;
            const bool ok = controller.images().size() == 6 && controller.importDetails().isEmpty();
            qInfo() << "Installed QML import: JPG/PNG/WebP/TIFF/DNG/HEIC, Unicode paths:" << ok;
            if (!ok) qCritical() << controller.importDetails();
            app.exit(ok ? 0 : 14);
        });
        QTimer::singleShot(0, &app, [&engine, urls, &app]() {
            const bool invoked = QMetaObject::invokeMethod(engine.rootObjects().value(0), "importSelection", Q_ARG(QVariant, QVariant(urls)));
            if (!invoked) app.exit(15);
        });
        QTimer::singleShot(60000, &app, [&app]() { app.exit(16); });
    }
    const bool advancedRenderTest = app.arguments().contains(QStringLiteral("--advanced-render-smoke-test"));
    if (app.arguments().contains(QStringLiteral("--render-smoke-test")) || advancedRenderTest) {
        if (!QImageWriter::supportedImageFormats().contains("webp")
            || !QImageReader::supportedImageFormats().contains("tiff")) {
            qCritical() << "Deployed image format plugins are missing";
            return 7;
        }
        renderFixture = std::make_unique<QTemporaryDir>();
        QImage testImage(600, 400, QImage::Format_RGB32);
        testImage.fill(QColor(60, 80, 100));
        const QString path = renderFixture->filePath(QStringLiteral("render-fixture.png"));
        if (!testImage.save(path)) return 3;
        QObject::connect(&controller, &AppController::currentImageChanged, &app, [&app, &engine, &controller, advancedRenderTest]() {
            if (advancedRenderTest) {
                controller.setSetting("cropLeft",25);
                controller.setSetting("rotation",1);
                controller.setSetting("hsl_blueLum",50);
                controller.setSetting("curveLights",20);
            }
            controller.setSetting(QStringLiteral("exposure"), 1.0);
            controller.endSettingEdit();
            QTimer::singleShot(3500, &app, [&app, &engine, &controller, advancedRenderTest]() {
                auto *window = qobject_cast<QQuickWindow*>(engine.rootObjects().value(0));
                auto *photo = window ? window->findChild<QQuickItem*>(QStringLiteral("photoLayer")) : nullptr;
                QImage frame = window ? window->grabWindow() : QImage();
                const QString capturePath = qEnvironmentVariable("TBRETOCH_SMOKE_SCREENSHOT");
                if (!capturePath.isEmpty() && !frame.isNull()) frame.save(capturePath);
                const QImage proxy(QUrl(advancedRenderTest ? controller.renderedPreviewUrl() : controller.currentPreviewUrl()).toLocalFile());
                if (!photo || frame.isNull() || proxy.isNull()) {
                    qCritical() << "Render smoke test could not capture the preview";
                    app.exit(4);
                    return;
                }
                const QPointF center = photo->mapToScene(QPointF(photo->width()/2, photo->height()/2));
                const double dpr = frame.devicePixelRatio();
                const QColor actual = frame.pixelColor(qBound(0, qRound(center.x()*dpr), frame.width()-1),
                                                       qBound(0, qRound(center.y()*dpr), frame.height()-1));
                const QColor source = proxy.pixelColor(proxy.width()/2, proxy.height()/2);
                const QColor expected = advancedRenderTest ? source : QColor(qMin(255, source.red()*2), qMin(255, source.green()*2), qMin(255, source.blue()*2));
                auto *exportButton = window->findChild<QQuickItem*>(QStringLiteral("exportButton"));
                const bool exportVisible = exportButton && exportButton->isVisible()
                    && exportButton->mapToScene(QPointF(exportButton->width(), 0)).x() <= window->width();
                auto *title = window->findChild<QQuickItem*>(QStringLiteral("slider-title-exposure"));
                const bool titleFits = title && title->width() >= title->implicitWidth();
                const bool geometryFits = !advancedRenderTest || (proxy.size() == QSize(400,450) && qRound(photo->width()) == 400 && qRound(photo->height()) == 450);
                const bool ok = geometryFits && exportVisible && titleFits && qAbs(actual.red()-expected.red()) <= 5
                    && qAbs(actual.green()-expected.green()) <= 5
                    && qAbs(actual.blue()-expected.blue()) <= 5;
                qInfo() << "Rendered exposure preview:" << actual << "expected:" << expected << "passed:" << ok;
                app.exit(ok ? 0 : 5);
            });
        });
        controller.importFiles({QUrl::fromLocalFile(path)});
        QTimer::singleShot(30000, &app, [&app]() { app.exit(6); });
    }
    if (app.arguments().contains(QStringLiteral("--smoke-test"))) {
        if (!engine.rootObjects().isEmpty()) engine.rootObjects().first()->setProperty("visible", false);
        QTimer::singleShot(1500, &app, [&app, &engine]() {
            const bool ok = !engine.rootObjects().isEmpty()
                && QFile::exists(QStringLiteral(":/shaders/color.vert.qsb"))
                && QFile::exists(QStringLiteral(":/shaders/color.frag.qsb"));
            qInfo() << "Native startup and shader resources:" << ok;
            app.exit(ok ? 0 : 2);
        });
    }
    return app.exec();
}
