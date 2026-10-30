#ifndef SDIOMODERN_OFFLINE_PATHS_H
#define SDIOMODERN_OFFLINE_PATHS_H
#include <QString>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QSaveFile>
#include <QDateTime>
#include <stdexcept>
inline QString libraryFamily(const QString &drivers,const QString &indexes){
    const QString leaf=QFileInfo(indexes).fileName();if(leaf.compare(QStringLiteral("SDI"),Qt::CaseInsensitive)==0)return QStringLiteral("SDI");if(leaf.compare(QStringLiteral("SDIO"),Qt::CaseInsensitive)==0)return QStringLiteral("SDIO");
    const QDir indexRoot(indexes);const bool onlySdi=indexRoot.exists(QStringLiteral("SDI")),onlySdio=indexRoot.exists(QStringLiteral("SDIO"));if(onlySdi!=onlySdio)return onlySdi?QStringLiteral("SDI"):QStringLiteral("SDIO");
    const QDir root(QFileInfo(drivers).absoluteDir());bool sdi=false,sdio=false;
    for(const QString &file:root.entryList(QStringList()<<QStringLiteral("*.exe"),QDir::Files)){if(file.startsWith(QStringLiteral("SDIO"),Qt::CaseInsensitive))sdio=true;else if(file.startsWith(QStringLiteral("SDI"),Qt::CaseInsensitive))sdi=true;}
    return sdi && !sdio?QStringLiteral("SDI"):sdio && !sdi?QStringLiteral("SDIO"):QStringLiteral("Automatic");
}
inline QString resolveIndexDirectory(const QString &path,const QString &drivers=QString())
{
    QString base=path;const QString leaf=QFileInfo(path).fileName().toLower();
    if(!QDir(base).exists() && (leaf==QStringLiteral("indexes") || leaf==QStringLiteral("index"))){const QDir parent=QFileInfo(path).absoluteDir();const QStringList aliases=QStringList()<<QStringLiteral("Index")<<QStringLiteral("indexes")<<QStringLiteral("Indexes");for(const QString &alias:aliases){const QString candidate=parent.filePath(alias);if(QDir(candidate).exists()){base=candidate;break;}}}
    const QString family=libraryFamily(drivers,base);
    QStringList children;if(family==QStringLiteral("SDI"))children<<QStringLiteral("SDI")<<QString()<<QStringLiteral("SDIO");else if(family==QStringLiteral("SDIO"))children<<QStringLiteral("SDIO")<<QString()<<QStringLiteral("SDI");else children<<QString()<<QStringLiteral("SDIO")<<QStringLiteral("SDI");
    QString best;int bestScore=-1;
    for(const QString &child:children){
        QDir dir(child.isEmpty()?base:QDir(base).filePath(child));
        const QStringList indexes=dir.entryList(QStringList()<<QStringLiteral("*.bin"),QDir::Files|QDir::Hidden|QDir::System);if(indexes.isEmpty())continue;
        if(family!=QStringLiteral("Automatic"))return dir.absolutePath();
        int score=0;for(const QString &file:indexes){QString name=QFileInfo(file).completeBaseName();if(name.startsWith(QStringLiteral("_P_")))name.replace(0,3,QStringLiteral("DP_"));if(QFileInfo(QDir(drivers).filePath(name+QStringLiteral(".7z"))).isFile())++score;}
        if(score>bestScore){bestScore=score;best=dir.absolutePath();}
    }
    if(!best.isEmpty())return best;const QString child=QDir(base).filePath(family);return family!=QStringLiteral("Automatic") && QDir(child).exists()?QFileInfo(child).absoluteFilePath():base;
}
inline bool compatibleLibraryIndex(const QString &file){QFile input(file);if(!input.open(QIODevice::ReadOnly))return false;const QByteArray h=input.read(7);if(h.size()!=7 || h.left(3)!="SDW")return false;const unsigned version=static_cast<unsigned char>(h[3])|(static_cast<unsigned char>(h[4])<<8)|(static_cast<unsigned char>(h[5])<<16)|(static_cast<unsigned char>(h[6])<<24);return version==0x204 || version==0x205;}
inline QString prepareLibraryIndexes(const QString &drivers,const QString &indexes){
    const QString source=resolveIndexDirectory(indexes,drivers);const QByteArray identity=(QFileInfo(drivers).absoluteFilePath().toLower()+QLatin1Char('|')+QFileInfo(source).absoluteFilePath().toLower()).toUtf8();
    const QString key=QString::fromLatin1(QCryptographicHash::hash(identity,QCryptographicHash::Sha256).toHex().left(24));
    const QString destination=QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)).filePath(QStringLiteral("IndexCache/")+key);
    if(!QDir().mkpath(destination))throw std::runtime_error("Cannot create the application's private index cache.");
    const QDir folder(source);for(const QString &name:folder.entryList(QStringList()<<QStringLiteral("*.bin"),QDir::Files|QDir::Hidden|QDir::System)){
        const QString inputPath=folder.filePath(name);if(!compatibleLibraryIndex(inputPath))continue;const QString outputPath=QDir(destination).filePath(name);
        const QFileInfo inputInfo(inputPath),outputInfo(outputPath);if(outputInfo.exists() && outputInfo.size()==inputInfo.size() && outputInfo.lastModified()>=inputInfo.lastModified())continue;
        QFile input(inputPath);QSaveFile output(outputPath);if(!input.open(QIODevice::ReadOnly) || !output.open(QIODevice::WriteOnly))throw std::runtime_error("Cannot copy a library index into the private cache.");
        while(!input.atEnd()){const QByteArray block=input.read(65536);if(block.isEmpty() && input.error()!=QFile::NoError)throw std::runtime_error("Cannot read a library index.");if(output.write(block)!=block.size())throw std::runtime_error("Cannot write a private library index.");}if(!output.commit())throw std::runtime_error("Cannot commit a private library index.");
    }return destination;
}
#endif
