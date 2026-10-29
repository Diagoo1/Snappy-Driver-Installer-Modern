#ifndef SDIO_APP_SETTINGS_H
#define SDIO_APP_SETTINGS_H
#include <QSettings>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QCryptographicHash>

// Defaults live in the executable; mutable preferences belong to this account.
inline QSettings *createAppSettings(const QString &root, QObject *parent=0, bool migrate=true)
{
    const QString organization=QCoreApplication::organizationName().isEmpty()
        ?QStringLiteral("TarekSadek"):QCoreApplication::organizationName();
    QSettings *settings=new QSettings(QSettings::NativeFormat,QSettings::UserScope,
        organization,QStringLiteral("SnappyDriverInstaller-TS"),parent);
    settings->setFallbacksEnabled(false);
    if(!migrate)return settings;
    // Read the older generation first so current preferences win if both exist.
    // Remove each sidecar only after its contents have been persisted.
    const QStringList names=QStringList()<<QStringLiteral("TarekSadekDriverSuite.ini")
        <<QStringLiteral("SnappyDriverInstaller-TS.ini");
    for(const QString &name:names){
        const QString path=QDir(root).filePath(name);
        QFile input(path);if(!input.exists())continue;
        if(!input.open(QIODevice::ReadOnly))continue;
        const QByteArray contents=input.readAll();input.close();
        const QString marker=QStringLiteral("legacyImports/")+QString::fromLatin1(
            QCryptographicHash::hash(QFileInfo(path).absoluteFilePath().toUtf8(),QCryptographicHash::Sha256).toHex());
        const QByteArray digest=QCryptographicHash::hash(contents,QCryptographicHash::Sha256).toHex();
        if(settings->value(marker).toByteArray()!=digest){
            QSettings previous(path,QSettings::IniFormat);previous.setFallbacksEnabled(false);
            const QStringList keys=previous.allKeys();
            if(previous.status()!=QSettings::NoError)continue;
            for(const QString &key:keys)settings->setValue(key,previous.value(key));
            settings->setValue(marker,digest);
        }
        settings->sync();
        if(settings->status()==QSettings::NoError)QFile::remove(path);
        else break;
    }
    return settings;
}
#endif
