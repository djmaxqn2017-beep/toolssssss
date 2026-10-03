#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

class LocalizationManager final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(QStringList availableLanguages READ availableLanguages CONSTANT)

public:
    explicit LocalizationManager(QObject *parent = nullptr);

    QString language() const;
    void setLanguage(const QString &language);
    QStringList availableLanguages() const;

    Q_INVOKABLE QString t(const QString &key) const;

signals:
    void languageChanged();

private:
    QString m_language = QStringLiteral("vi");
};
