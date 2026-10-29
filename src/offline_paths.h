#ifndef SDIOMODERN_OFFLINE_PATHS_H
#define SDIOMODERN_OFFLINE_PATHS_H
#include <QString>
#include <QDir>
inline QString resolveIndexDirectory(const QString &path)
{
    // SDI distributes indexes/SDI; Origin commonly uses indexes/SDIO.
    const QStringList children=QStringList()<<QString()<<QStringLiteral("SDIO")<<QStringLiteral("SDI");
    for(const QString &child:children){
        QDir dir(child.isEmpty()?path:QDir(path).filePath(child));
        if(!dir.entryList(QStringList()<<QStringLiteral("*.bin"),QDir::Files|QDir::Hidden|QDir::System).isEmpty())
            return dir.absolutePath();
    }
    return path;
}
#endif
