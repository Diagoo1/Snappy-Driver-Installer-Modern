#ifndef SDIO_SUPPORT_REPORT_H
#define SDIO_SUPPORT_REPORT_H
#include <QJsonObject>
#include <QString>
#include "driver_catalog.h"
QJsonObject supportReport(const DriverCatalog &catalog,const sdiomodern::InstallReport &installation,const QString &lastError);
#endif
