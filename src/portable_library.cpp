#include "portable_library.h"
#include "appearance.h"
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QDir>
#include <QDirIterator>
#include <QCryptographicHash>
#include <QStorageInfo>
#include <QCoreApplication>
#include <QThread>
#include <QTimer>
#include <QVBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QCheckBox>
#include <QFileDialog>
#include <stdexcept>

namespace PortableLibrary {
static void fail(const char *message){throw std::runtime_error(message);}
void copy(const QVector<Entry> &files,const QString &destination,const std::atomic<bool> &stop,const std::function<void(qint64,qint64)> &progress){
    if(files.isEmpty())fail("No library files were selected.");
    if(QFileInfo(destination).isSymLink() || !QDir().mkpath(destination))fail("Choose a regular writable destination.");
    QDir root(destination);const QString canonical=QFileInfo(destination).canonicalFilePath();qint64 total=0,done=0;
    for(const Entry &e:files){const QString path=QDir::cleanPath(e.relative);
        if(path!=e.relative || path.startsWith(QLatin1Char('/')) || path.contains(QLatin1Char(':')) || path.contains(QLatin1Char('\\')) || path==QStringLiteral("..") || path.startsWith(QStringLiteral("../")))fail("Invalid portable library path.");
        if(!QFileInfo(e.source).isFile() || QFileInfo(e.source).isSymLink())fail("A source file is unavailable.");total+=QFileInfo(e.source).size();}
    QStorageInfo volume(destination);if(volume.isValid() && volume.isReady() && volume.bytesAvailable()<total+32*1024*1024)fail("The destination needs more free space.");
    for(const Entry &e:files){
        if(stop.load())fail("Portable copy cancelled. Completed files were kept.");
        const QString target=root.filePath(e.relative);QFileInfo info(target);if(info.isSymLink())fail("Cannot prepare the destination.");
        QString current=canonical;const QStringList parts=e.relative.split(QLatin1Char('/'));
        for(int i=0;i<parts.size()-1;++i){current=QDir(current).filePath(parts[i]);if(QFileInfo(current).isSymLink())fail("Destination contains a symbolic link.");if(!QDir().mkpath(current))fail("Cannot create a destination folder.");}
        const QString parent=QFileInfo(info.absolutePath()).canonicalFilePath();if(parent.compare(canonical,Qt::CaseInsensitive)!=0 && !parent.startsWith(canonical+QLatin1Char('/'),Qt::CaseInsensitive))fail("Destination leaves the portable folder.");
        if(QFileInfo(e.source).canonicalFilePath().compare(info.canonicalFilePath(),Qt::CaseInsensitive)==0)fail("Source and destination must be different.");
        QFile source(e.source);QSaveFile output(target);QCryptographicHash original(QCryptographicHash::Sha256);
        if(!source.open(QIODevice::ReadOnly) || !output.open(QIODevice::WriteOnly))fail("Cannot open a library file.");
        while(!source.atEnd()){if(stop.load())fail("Portable copy cancelled. Completed files were kept.");const QByteArray bytes=source.read(1024*1024);if(bytes.isEmpty() && source.error()!=QFile::NoError)fail("Source read failed.");if(output.write(bytes)!=bytes.size())fail("Destination write failed.");original.addData(bytes);done+=bytes.size();if(progress)progress(done,total);}
        if(!output.commit())fail("Cannot finish the portable file.");
        QFile result(target);QCryptographicHash verified(QCryptographicHash::Sha256);if(!result.open(QIODevice::ReadOnly))fail("Cannot verify the copied file.");
        while(!result.atEnd()){if(stop.load())fail("Portable copy cancelled. Completed files were kept.");const QByteArray bytes=result.read(1024*1024);if(bytes.isEmpty() && result.error()!=QFile::NoError)fail("Verification read failed.");verified.addData(bytes);}
        if(original.result()!=verified.result())fail("A copied file failed verification.");
    }
}
}
class PortableWorker : public QThread {
public:explicit PortableWorker(QObject *parent):QThread(parent),stop(false),done(0),total(0){}
    QVector<PortableLibrary::Entry> files;QString destination,error;std::atomic<bool> stop;std::atomic<qint64> done,total;
protected:void run() Q_DECL_OVERRIDE {try{PortableLibrary::copy(files,destination,stop,[this](qint64 n,qint64 all){done.store(n);total.store(all);});}catch(const std::exception &e){error=QString::fromUtf8(e.what());}}
};
void openPortableLibrary(QWidget *parent,const QString &drivers,const QString &indexes,const QStringList &needed,bool ar){
    auto t=[ar](const char *en,const char *arabic){return ar?QString::fromUtf8(arabic):QString::fromUtf8(en);};
    RoundedDialog dialog(parent);dialog.setMinimumWidth(610);dialog.setWindowTitle(t("Portable library","مكتبة محمولة"));QVBoxLayout *frame=new QVBoxLayout(&dialog);frame->setContentsMargins(0,0,0,0);frame->setSpacing(0);frame->addWidget(new DialogHeader(t("Create a portable library","إنشاء مكتبة محمولة"),QStringLiteral("usb"),&dialog,ar));
    QWidget *body=new QWidget(&dialog);QVBoxLayout *box=new QVBoxLayout(body);box->setContentsMargins(25,22,25,25);box->setSpacing(15);frame->addWidget(body);
    QLabel *intro=new QLabel(t("Copy the program, packages and indexes to a USB drive or another folder. Unrelated files are kept. No drive is formatted.","انسخ البرنامج والحزم والفهارس إلى USB أو مجلد آخر. الملفات الأخرى تبقى كما هي، دون تهيئة أي قرص."),body);intro->setWordWrap(true);intro->setObjectName(QStringLiteral("bodyText"));box->addWidget(intro);
    QCheckBox *onlyNeeded=new QCheckBox(t("Only packages recommended for this computer","الحزم الموصى بها لهذا الجهاز فقط"),body);onlyNeeded->setEnabled(!needed.isEmpty());onlyNeeded->setChecked(!needed.isEmpty());box->addWidget(onlyNeeded);
    ActionButton *choose=new ActionButton(t("Choose destination & copy","اختيار الوجهة والنسخ"),body);choose->setObjectName(QStringLiteral("primaryButton"));box->addWidget(choose);
    QProgressBar *bar=new QProgressBar(body);bar->setObjectName(QStringLiteral("mainProgress"));bar->setRange(0,1000);bar->setValue(0);box->addWidget(bar);
    QLabel *status=new QLabel(t("One executable. Your offline library.","ملف تشغيلي واحد، ومكتبتك الأوفلاين."),body);status->setObjectName(QStringLiteral("subText"));status->setWordWrap(true);box->addWidget(status);
    PortableWorker worker(&dialog);QTimer timer;timer.setInterval(100);QObject::connect(&timer,&QTimer::timeout,&dialog,[&](){const qint64 all=worker.total.load();if(all)bar->setValue(static_cast<int>(worker.done.load()*1000/all));});
    QObject::connect(choose,&QPushButton::clicked,&dialog,[&](){
        const QString folder=QFileDialog::getExistingDirectory(&dialog,t("Choose USB drive or folder","اختيار USB أو مجلد"));if(folder.isEmpty())return;
        worker.files.clear();worker.destination=QDir(folder).filePath(QStringLiteral("Snappy Driver Installer"));worker.stop.store(false);worker.error.clear();worker.done.store(0);worker.total.store(0);
        auto add=[&](const QString &source,const QString &relative){PortableLibrary::Entry e;e.source=source;e.relative=relative;worker.files.append(e);};
        add(QCoreApplication::applicationFilePath(),QStringLiteral("SnappyDriverInstaller-TS.exe"));const QDir app(QCoreApplication::applicationDirPath());
        const QStringList supportingFolders=QStringList()<<QStringLiteral("langs")<<QStringLiteral("licenses")<<QStringLiteral("themes");
        for(const QString &name:supportingFolders){QDirIterator it(app.filePath(name),QDir::Files|QDir::NoSymLinks,QDirIterator::Subdirectories);while(it.hasNext()){const QString path=it.next();add(path,app.relativeFilePath(path));}}
        if(QFileInfo(app.filePath(QStringLiteral("LICENSE-GPL-3.0.txt"))).isFile())add(app.filePath(QStringLiteral("LICENSE-GPL-3.0.txt")),QStringLiteral("LICENSE-GPL-3.0.txt"));
        QDir library(drivers);for(const QString &name:library.entryList(QStringList()<<QStringLiteral("*.7z"),QDir::Files|QDir::NoSymLinks))if(!onlyNeeded->isChecked() || needed.contains(name,Qt::CaseInsensitive))add(library.filePath(name),QStringLiteral("drivers/")+name);
        QDir index(indexes);for(const QString &name:index.entryList(QStringList()<<QStringLiteral("*.bin"),QDir::Files|QDir::NoSymLinks))add(index.filePath(name),QStringLiteral("indexes/SDIO/")+name);
        choose->setEnabled(false);onlyNeeded->setEnabled(false);status->setText(t("Copying and verifying files...","جارٍ نسخ الملفات والتحقق منها..."));timer.start();worker.start();
    });
    QObject::connect(&worker,&QThread::finished,&dialog,[&](){timer.stop();choose->setEnabled(true);onlyNeeded->setEnabled(!needed.isEmpty());status->setText(worker.error.isEmpty()?t("Portable library ready: ","المكتبة المحمولة جاهزة: ")+worker.destination:worker.error);});
    dialog.exec();worker.stop.store(true);worker.wait();
}
