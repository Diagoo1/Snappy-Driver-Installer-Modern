#ifndef SDIO_UPDATE_POLICY_H
#define SDIO_UPDATE_POLICY_H
#include <QSettings>
#include <QDateTime>
namespace UpdatePolicy {
inline QString family(const QSettings &settings,const QString &detected){
    const QString selected=settings.value(QStringLiteral("updates/source"),QStringLiteral("auto")).toString();
    return selected==QStringLiteral("SDI") || selected==QStringLiteral("SDIO")?selected:detected==QStringLiteral("SDI")?QStringLiteral("SDI"):QStringLiteral("SDIO");
}
inline QString mode(const QSettings &settings){return settings.value(QStringLiteral("updates/mode"),settings.value(QStringLiteral("updates/checkAtStartup"),true).toBool()?QStringLiteral("startup"):QStringLiteral("manual")).toString();}
inline bool showNotice(const QString &mode,bool checkedThisSession){return mode!=QStringLiteral("never") && (mode!=QStringLiteral("manual") || checkedThisSession);}
inline QString key(const QString &family,const char *field){return QStringLiteral("updates/history/")+family+QLatin1Char('/')+QString::fromLatin1(field);}
inline bool due(const QSettings &settings,const QString &family,const QDateTime &now,bool startup){
    const QString selected=mode(settings);if(selected==QStringLiteral("manual") || selected==QStringLiteral("never"))return false;
    if(selected==QStringLiteral("startup"))return startup;
    const QDateTime attempt=settings.value(key(family,"attempt")).toDateTime();
    if(attempt.isValid() && attempt.secsTo(now)<900)return false;
    const QDateTime success=settings.value(key(family,"success")).toDateTime();
    if(selected==QStringLiteral("launches"))return startup && settings.value(QStringLiteral("updates/launchesSinceCheck"),0).toInt()>=qBound(1,settings.value(QStringLiteral("updates/launches"),3).toInt(),100);
    if(selected==QStringLiteral("interval"))return !success.isValid() || success.secsTo(now)>=qint64(qBound(1,settings.value(QStringLiteral("updates/hours"),24).toInt(),720))*3600;
    if(selected==QStringLiteral("date")){const QDateTime scheduled=settings.value(QStringLiteral("updates/date")).toDateTime();return scheduled.isValid() && scheduled<=now && (!success.isValid() || success<scheduled);}
    return false;
}
inline void succeeded(QSettings &settings,const QString &family,const QDateTime &now){settings.setValue(key(family,"success"),now);settings.setValue(QStringLiteral("updates/launchesSinceCheck"),0);settings.sync();}
}
#endif
