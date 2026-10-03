#pragma once
#include <QString>
#include <QSize>
#include <QStringList>
#include <QByteArray>
bool runMetadataTool(const QString &executable,const QStringList &arguments,QByteArray *output,QString *error);
bool preserveExportMetadata(const QString &source,const QString &destination,QSize size,QString *error);
