#pragma once
#include <QImage>
#include <QVariantMap>

QVariantMap advancedDefaults();
bool hasAdvancedSettings(const QVariantMap &settings);
QImage applyAdvancedRecipe(QImage image, const QVariantMap &settings);
bool isGeometryKey(const QString &key);
