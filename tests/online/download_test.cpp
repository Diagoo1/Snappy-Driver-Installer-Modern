#include "../../src/online_packages.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include <QMap>
#include <QJsonObject>
#include <cstdio>
#include <stdexcept>
using namespace OnlinePackages;
static int checks=0;
static void check(bool ok,const char *message){if(!ok)throw std::runtime_error(message);++checks;}
#include "fixture.h"
template<class F>static bool rejects(F action){try{action();return false;}catch(const std::exception&){return true;}}
static QByteArray read(const QString &path){QFile file(path);if(!file.open(QIODevice::ReadOnly))return QByteArray();return file.readAll();}
int main(int argc,char **argv){QCoreApplication app(argc,argv);
    try{
        Fixture f;Catalog c=parse(f.torrent);check(c.files.size()==4,"all source files are represented for cross-file verification");
        check(c.files[1].kind==Index && c.files[2].kind==Driver && c.files[3].kind==Other,"only approved file types are publishable");
        check(c.files[2].offset==21000,"torrent offsets include unselected neighboring files");
        check(requiredBytes(c,QVector<int>()<<1)==32768,"selected index requires its complete boundary blocks");
        check(requiredBytes(c,QVector<int>()<<1<<1)==32768,"duplicate selections do not duplicate transfer size");
        QTemporaryDir directory;check(directory.isValid(),"temporary test workspace");QDir root(directory.path());
        const QString drivers=root.filePath(QStringLiteral("drivers")),indexes=root.filePath(QStringLiteral("indexes")),cache=root.filePath(QStringLiteral("cache"));
        std::atomic<bool> stop(false);qint64 transferred=0;const RangeFetch fetch=[&](const QUrl &u,qint64 p,qint64 n,qint64 total){transferred+=n;return f.fetch(u,p,n,total);};
        QStringList paths=download(c,QVector<int>()<<1,drivers,indexes,cache,stop,Sink(),fetch);
        check(paths.size()==1 && read(root.filePath(QStringLiteral("indexes/_P_Test.bin")))==f.data[1],"cross-file piece verification publishes the exact selected index");
        check(!QFile::exists(root.filePath(QStringLiteral("tools/ignored.exe"))),"neighboring executable bytes never become executable files");
        check(transferred==32768,"only required verified ranges are requested");
        check(rejects([&](){download(c,QVector<int>()<<3,drivers,indexes,cache,stop,Sink(),fetch);}),"executable selection is rejected");
        const QByteArray previous=read(root.filePath(QStringLiteral("indexes/_P_Test.bin")));
        RangeFetch corrupt=[&](const QUrl &u,qint64 p,qint64 n,qint64 total){QByteArray bytes=f.fetch(u,p,n,total);if(!bytes.isEmpty())bytes[0]=static_cast<char>(bytes[0]^1);return bytes;};
        check(rejects([&](){download(c,QVector<int>()<<1,drivers,indexes,cache,stop,Sink(),corrupt);}),"hash mismatch rejects the download source");
        check(read(root.filePath(QStringLiteral("indexes/_P_Test.bin")))==previous,"failed integrity check preserves an existing file");
        transferred=0;check(rejects([&](){download(c,QVector<int>()<<1<<2,drivers,indexes,cache,stop,[&](const Progress &p){if(p.verified>=16384)stop.store(true);},fetch);}),"download pause stops before publishing unverified files");
        check(!QFile::exists(root.filePath(QStringLiteral("drivers/DP_Test.7z"))),"pause preserves destination until all required blocks pass verification");
        stop.store(false);transferred=0;paths=download(c,QVector<int>()<<1<<2,drivers,indexes,cache,stop,Sink(),fetch);
        check(read(root.filePath(QStringLiteral("drivers/DP_Test.7z")))==f.data[2],"resumed download publishes exact driver archive data");
        check(transferred<requiredBytes(c,QVector<int>()<<1<<2),"resume reuses verified cached blocks");
        Fixture bad;bad.names[1]="indexes/SDIO/../DP_Test.bin";bad.rebuild();check(rejects([&](){parse(bad.torrent);}),"parent traversal rejected");
        bad=Fixture();bad.names[1]="indexes/SDIO/CON.bin";bad.rebuild();check(rejects([&](){parse(bad.torrent);}),"Windows device names rejected");
        bad=Fixture();bad.names[3]="DRIVERS/dp_test.7Z";bad.rebuild();check(rejects([&](){parse(bad.torrent);}),"case-insensitive duplicate paths rejected");
        bad=Fixture();bad.names[1]="indexes/SDIO/DP_Percent%20#name.bin";bad.rebuild();Catalog quoted=parse(bad.torrent);paths=download(quoted,QVector<int>()<<1,drivers,indexes,cache,stop,Sink(),[&](const QUrl &u,qint64 p,qint64 n,qint64 total){return bad.fetch(u,p,n,total);});
        check(read(paths.first())==bad.data[1],"URL paths preserve literal percent, fragment characters and spaces");
        check(rejects([&](){parse(f.torrent+'x');}),"trailing metadata rejected");
        check(rejects([&](){parse(QByteArray(8*1024*1024+1,'x'));}),"oversized metadata rejected");
        Catalog changed=c;changed.hashes.clear();check(rejects([&](){parse(QByteArray("d4:infodee"));}),"missing torrent fields rejected");
        RangeFetch missingNeighbor=[&](const QUrl &u,qint64 p,qint64 n,qint64 total){if(u.path().endsWith(QStringLiteral("docs/readme.txt")))throw std::runtime_error("HTTP 404");return f.fetch(u,p,n,total);};
        check(rejects([&](){download(c,QVector<int>()<<1,drivers,indexes,cache,stop,Sink(),missingNeighbor);}),"unavailable neighboring block cannot bypass verification");
        check(QFile::exists(QDir(cache).filePath(QString::fromLatin1(c.infoHash.toHex())+QStringLiteral("/1.block"))),"available HTTP blocks are retained even if an earlier boundary block is unavailable");
        transferred=0;download(c,QVector<int>()<<1,drivers,indexes,cache,stop,Sink(),fetch);
        check(transferred==16384,"resuming retries only the unavailable boundary block");
        Catalog invalidLimits=c;invalidLimits.transfer.downloadKiB=-1;check(rejects([&](){download(invalidLimits,QVector<int>()<<1,drivers,indexes,cache,stop,Sink(),fetch);}),"negative bandwidth limit rejected");
        invalidLimits=c;invalidLimits.transfer.connections=0;check(rejects([&](){download(invalidLimits,QVector<int>()<<1,drivers,indexes,cache,stop,Sink(),fetch);}),"invalid connection count rejected");
        invalidLimits=c;invalidLimits.transfer.port=80;check(rejects([&](){download(invalidLimits,QVector<int>()<<1,drivers,indexes,cache,stop,Sink(),fetch);}),"privileged peer port rejected");
        Fixture peerOnly;peerOnly.peerOnly=true;peerOnly.rebuild();check(parse(peerOnly.torrent).seeds.isEmpty(),"peer-only catalog does not require an HTTP seed");
        stop.store(true);check(rejects([&](){download(c,QVector<int>()<<1,drivers,indexes,cache,stop,Sink(),fetch);}),"cancelled request never starts a transfer");
        std::printf("%d verified online-package checks passed; no network or installation used.\n",checks);return 0;
    }catch(const std::exception &e){std::fprintf(stderr,"Online test failed: %s\n",e.what());return 1;}
}
