#ifndef SDIO_THEME_PREFERENCE_H
#define SDIO_THEME_PREFERENCE_H
#include <QString>
#include <QSettings>
#include <QSysInfo>
namespace ThemePreference {
inline bool resolve(const QString &mode,bool modernWindows,bool appsUseLightTheme){if(mode==QStringLiteral("dark"))return true;if(mode==QStringLiteral("light"))return false;return modernWindows && !appsUseLightTheme;}
inline QString mode(const QSettings &settings){const QString value=settings.value(QStringLiteral("theme/mode"),QStringLiteral("system")).toString();return value==QStringLiteral("dark") || value==QStringLiteral("light")?value:QStringLiteral("system");}
inline bool systemDark(){
#ifdef Q_OS_WIN
    if(QSysInfo::windowsVersion()<QSysInfo::WV_WINDOWS10)return false;
    QSettings personalize(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"),QSettings::NativeFormat);personalize.setFallbacksEnabled(false);
    return resolve(QStringLiteral("system"),true,personalize.value(QStringLiteral("AppsUseLightTheme"),1).toInt()!=0);
#else
    return false;
#endif
}
inline bool effective(const QString &mode){return mode==QStringLiteral("system")?systemDark():mode==QStringLiteral("dark");}
}
#endif
