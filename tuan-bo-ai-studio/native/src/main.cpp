#include <QGuiApplication>
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
    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("TB"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("tbretouch.local"));
    QCoreApplication::setApplicationName(QStringLiteral("TBRetoch"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.6.0-dev"));

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
    if (app.arguments().contains(QStringLiteral("--render-smoke-test"))) {
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
        QObject::connect(&controller, &AppController::currentImageChanged, &app, [&app, &engine, &controller]() {
            controller.setSetting(QStringLiteral("exposure"), 1.0);
            controller.endSettingEdit();
            QTimer::singleShot(2500, &app, [&app, &engine, &controller]() {
                auto *window = qobject_cast<QQuickWindow*>(engine.rootObjects().value(0));
                auto *photo = window ? window->findChild<QQuickItem*>(QStringLiteral("photoLayer")) : nullptr;
                QImage frame = window ? window->grabWindow() : QImage();
                const QString capturePath = qEnvironmentVariable("TBRETOCH_SMOKE_SCREENSHOT");
                if (!capturePath.isEmpty() && !frame.isNull()) frame.save(capturePath);
                const QImage proxy(QUrl(controller.currentPreviewUrl()).toLocalFile());
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
                const QColor expected(qMin(255, source.red()*2), qMin(255, source.green()*2), qMin(255, source.blue()*2));
                const bool ok = qAbs(actual.red()-expected.red()) <= 5
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
