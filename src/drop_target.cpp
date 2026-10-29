#include "drop_target.h"
#include <QFileInfo>
DropTarget::Kind DropTarget::classify(const QList<QUrl> &urls,QString *path){
    if(urls.size()!=1 || !urls.first().isLocalFile())return Rejected;
    const QFileInfo file(urls.first().toLocalFile());if(!file.exists() || file.isSymLink())return Rejected;
    const Kind kind=file.isDir()?DriverFolder:(file.isFile() && file.suffix().compare(QStringLiteral("snp"),Qt::CaseInsensitive)==0 && file.size()>0 && file.size()<=128*1024*1024?SnapshotFile:Rejected);
    if(kind!=Rejected && path)*path=file.canonicalFilePath();return kind;
}
