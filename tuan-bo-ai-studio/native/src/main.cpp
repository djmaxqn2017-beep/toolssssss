#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QSGRendererInterface>

#include "AppController.h"

int main(int argc, char *argv[]) {
#if defined(Q_OS_WIN)
    QQuickWindow::setGraphicsApi(QSGRendererInterface::Direct3D11);
#endif
    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("TB"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("tbretouch.local"));
    QCoreApplication::setApplicationName(QStringLiteral("TBRetoch"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.5.0"));

    AppController controller;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("appController"), &controller);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app, []() {
        QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.loadFromModule("TBRetoch", "Main");
    return app.exec();
}
