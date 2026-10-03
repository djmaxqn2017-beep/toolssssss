#pragma once
#include <QImage>
#include <QString>

// One decoder for both previews and original-resolution exports.
QImage decodeImage(const QString &path, int maxSide, QString *error);
