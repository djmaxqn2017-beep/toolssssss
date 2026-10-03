#include <QGuiApplication>
#include <QTimer>
#include <QFile>
#include <QDebug>
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
