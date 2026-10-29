#ifndef SDIO_PORTABLE_LIBRARY_H
#define SDIO_PORTABLE_LIBRARY_H
#include <QString>
#include <QStringList>
#include <QVector>
#include <atomic>
#include <functional>
class QWidget;
namespace PortableLibrary {
struct Entry {QString source,relative;};
void copy(const QVector<Entry>&,const QString&,const std::atomic<bool>&,const std::function<void(qint64,qint64)>&);
}
void openPortableLibrary(QWidget*,const QString&,const QString&,const QStringList&,bool);
#endif
