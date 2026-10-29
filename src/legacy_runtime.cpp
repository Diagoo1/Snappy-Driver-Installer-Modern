#include "legacy_script.h"
#include "online_packages.h"
#include "app_update.h"
#include "app_settings.h"
#include "theme.h"
#include "offline_paths.h"
#include "../native/origin_api.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QProcess>
#include <QScopedPointer>
#include <QSettings>
#include <QDateTime>
#include <QTextStream>
#include <QTemporaryDir>
#include <QRegularExpression>
#include <cstdio>
#include <stdexcept>
#ifdef Q_OS_WIN
#include <windows.h>
#include <objbase.h>
#endif
namespace Legacy {
namespace {
void fail(const QString &text){throw std::runtime_error(text.toUtf8().constData());}
void saveJson(const QString &path,const QJsonObject &value){if(path.isEmpty())return;QSaveFile file(path);const QByteArray bytes=QJsonDocument(value).toJson();if(!file.open(QIODevice::WriteOnly) || file.write(bytes)!=bytes.size() || !file.commit())fail(QStringLiteral("Cannot write the legacy report."));}
QString pathArgument(const Command &command){return command.arguments.join(QStringLiteral(" "));}
struct Runtime {
    sdiomodern::Paths paths;
    std::unique_ptr<sdiomodern::OriginCore> core;
    sdiomodern::Snapshot snapshot;
    QVector<QPair<QString,quint64> > selected;
    QStringList selectedPackages;
    QStringList selectionCriteria;
    QString extraction,repository,stagedApp,installedIdLog;
    bool installedIdLogStarted=false;
    OnlinePackages::Catalog torrent;
    OnlinePackages::TransferSettings transfer;
    bool updatesReady=false,enableInstall=true,rebootRequired=false,extractOnly=false,restore=false,noStop=false;
    int activeTorrent=0,filterMask=-1;
    std::atomic<bool> stop;
    QJsonArray events;
    QJsonObject outcomes;
    Runtime():repository(AppUpdate::projectRepository()),stop(false){const QDir root(QCoreApplication::applicationDirPath());paths.drivers=root.filePath(QStringLiteral("drivers")).toStdWString();paths.indexes=root.filePath(QStringLiteral("indexes/SDIO")).toStdWString();paths.logDirectory=root.filePath(QStringLiteral("logs")).toStdWString();}
    sdiomodern::OriginCore &engine(){if(!core)core.reset(new sdiomodern::OriginCore);return *core;}
    sdiomodern::EventSink sink(){return [this](const sdiomodern::Event &event){QJsonObject item;item.insert(QStringLiteral("message"),QString::fromStdWString(event.message));item.insert(QStringLiteral("stage"),int(event.stage));item.insert(QStringLiteral("nativeError"),double(event.nativeError));events.append(item);};}
    void scan(){if(!QDir().mkpath(QString::fromStdWString(paths.logDirectory)))fail(QStringLiteral("Cannot create the requested index/log folders."));sdiomodern::Paths session=paths;session.indexes=prepareLibraryIndexes(QString::fromStdWString(paths.drivers),QString::fromStdWString(paths.indexes)).toStdWString();snapshot=engine().scan(session,sink());paths.forceReindex=false;selected.clear();selectedPackages.clear();if(!selectionCriteria.isEmpty())select(selectionCriteria);}
    void ensureScan(){if(!snapshot.generation)scan();}
    void select(const QStringList &arguments){
        ensureScan();selectionCriteria=arguments;selected.clear();selectedPackages.clear();unsigned mask=0;QStringList packages;bool all=false;
        const QStringList names=QStringList()<<QStringLiteral("missing")<<QStringLiteral("newer")<<QStringLiteral("current")<<QStringLiteral("older")<<QStringLiteral("better")<<QStringLiteral("worse");
        for(const QString &raw:arguments){const QString word=raw.toLower();const int flag=names.indexOf(word);if(flag>=0)mask|=1u<<flag;else if(word==QStringLiteral("all"))all=true;else if(word==QStringLiteral("none"))return;else packages<<raw;}
        if(!mask)mask=63;if(filterMask>=0)mask=unsigned(filterMask>>1);
        for(const auto &device:snapshot.devices){const sdiomodern::Candidate *best=0;
            for(const auto &candidate:device.candidates){const QString package=QString::fromStdWString(candidate.packageName);bool packageMatches=packages.isEmpty();for(const QString &filter:packages)if(package.contains(filter,Qt::CaseInsensitive))packageMatches=true;
                const bool visible=all || (mask/64<candidate.filterVisibility.size() && (candidate.filterVisibility[mask/64]&(std::uint64_t(1)<<(mask%64))));
                if(!candidate.valid || !visible || !packageMatches)continue;if(!best || candidate.originalOrder<best->originalOrder)best=&candidate;
            }
            if(best){selected.append(qMakePair(QString::fromStdWString(device.instanceId),quint64(best->token)));const QString pack=QString::fromStdWString(best->packageName);if(!selectedPackages.contains(pack,Qt::CaseInsensitive))selectedPackages<<pack;}
        }
    }
    sdiomodern::InstallPlan plan(bool installing){
        ensureScan();engine().setOperationOptions(paths);sdiomodern::InstallPlan result;result.generation=snapshot.generation;result.confirmed=true;result.createRestorePoint=restore;result.allowAlternatives=true;result.continueOnRestoreFailure=noStop;
        if(installing && !installedIdLog.isEmpty() && !installedIdLogStarted){QDir().mkpath(QFileInfo(installedIdLog).absolutePath());engine().setInstalledIdLog(installedIdLog.toStdWString());installedIdLogStarted=true;}
        for(const auto &entry:selected){sdiomodern::Selection item;item.deviceInstanceId=entry.first.toStdWString();item.candidateToken=entry.second;result.selections.push_back(item);}
        if(installing && (!enableInstall || !paths.snapshotFile.empty() || paths.targetVersion || paths.targetArchitecture))fail(QStringLiteral("Installation is disabled for this script or preview."));return result;
    }
    void checkUpdates(){const QUrl url=activeTorrent?QUrl(QStringLiteral("https://www.glenn.delahoy.com/downloads/sdio/Drivers.torrent")):OnlinePackages::officialMetadataUrl();torrent=OnlinePackages::parse(OnlinePackages::fetchMetadata(url,stop));torrent.transfer=transfer;updatesReady=true;}
    void download(const QString &target,const QString &choice){
        if(target==QStringLiteral("app")){
            const auto release=AppUpdate::latest(repository,stop);const QString folder=QDir(QString::fromStdWString(paths.logDirectory)).filePath(QStringLiteral("application-update"));if(!QDir().mkpath(folder))fail(QStringLiteral("Cannot create the application download folder."));stagedApp=QDir(folder).filePath(QStringLiteral("SnappyDriverInstaller-TS.exe"));AppUpdate::downloadVerified(release,stagedApp,stop);outcomes.insert(QStringLiteral("downloadedApplication"),stagedApp);return;
        }
        if(!updatesReady)checkUpdates();QVector<int> files;QStringList needed;
        if(target==QStringLiteral("driverpacks") && choice!=QStringLiteral("all")){
            ensureScan();if(choice==QStringLiteral("selected"))needed=selectedPackages;
            else for(const auto &device:snapshot.devices)for(const auto &candidate:device.candidates)if(candidate.valid && candidate.recommended && !candidate.local && (choice!=QStringLiteral("missing") || !device.hasInstalledDriver)){const QString name=QString::fromStdWString(candidate.packageName);if(!needed.contains(name,Qt::CaseInsensitive))needed<<name;}
        }
        for(int i=0;i<torrent.files.size();++i){const auto &file=torrent.files[i];if((target==QStringLiteral("indexes") && file.kind==OnlinePackages::Index) || (target==QStringLiteral("driverpacks") && file.kind==OnlinePackages::Driver && (choice==QStringLiteral("all") || needed.contains(file.targetName,Qt::CaseInsensitive))))files<<i;}
        if(files.isEmpty())return;const QString drivers=QString::fromStdWString(paths.drivers),indexes=QString::fromStdWString(paths.indexes);QDir().mkpath(drivers);QDir().mkpath(indexes);torrent.transfer=transfer;
        const QStringList published=OnlinePackages::download(torrent,files,drivers,indexes,QDir(drivers).filePath(QStringLiteral(".sdio-downloads")),stop,[](const OnlinePackages::Progress&){});outcomes.insert(QStringLiteral("downloadedFiles"),QJsonArray::fromStringList(published));snapshot=sdiomodern::Snapshot();
    }
    int invoke(const Command &command){
        try{const QString n=command.name;const QString first=command.arguments.value(0).toLower();const QString joined=pathArgument(command);
            if(n==QStringLiteral("init")){paths.forceReindex=first==QStringLiteral("reindex");scan();}
            else if(n==QStringLiteral("drpdir")){paths.drivers=QFileInfo(joined).absoluteFilePath().toStdWString();snapshot=sdiomodern::Snapshot();}
            else if(n==QStringLiteral("indexdir")){paths.indexes=resolveIndexDirectory(QFileInfo(joined).absoluteFilePath()).toStdWString();snapshot=sdiomodern::Snapshot();}
            else if(n==QStringLiteral("logdir"))paths.logDirectory=QFileInfo(joined).absoluteFilePath().toStdWString();
            else if(n==QStringLiteral("extractdir"))extraction=QFileInfo(joined).absoluteFilePath();
            else if(n==QStringLiteral("loadsnapshot")){if(!QFileInfo(joined).isFile())fail(QStringLiteral("Snapshot file does not exist."));paths.snapshotFile=QFileInfo(joined).absoluteFilePath().toStdWString();snapshot=sdiomodern::Snapshot();}
            else if(n==QStringLiteral("unloadsnapshot")){paths.snapshotFile.clear();snapshot=sdiomodern::Snapshot();}
            else if(n==QStringLiteral("select"))select(command.arguments);
            else if(n==QStringLiteral("snapshot")){ensureScan();const QString file=joined.isEmpty()?QDir(QString::fromStdWString(paths.logDirectory)).filePath(QStringLiteral("state.snp")):joined;engine().saveSnapshot(file.toStdWString());outcomes.insert(QStringLiteral("snapshot"),QFileInfo(file).absoluteFilePath());}
            else if(n==QStringLiteral("writedevicelist")){ensureScan();engine().writeDeviceList(joined.toStdWString());}
            else if(n==QStringLiteral("install")){
                const auto queue=plan(!extractOnly);if(queue.selections.empty())return 0;
                if(extractOnly){if(extraction.isEmpty() || !QDir().mkpath(extraction))fail(QStringLiteral("Choose a writable extraction directory."));outcomes.insert(QStringLiteral("extracted"),QString::fromStdWString(engine().extract(queue,extraction.toStdWString(),sink(),stop)));}
                else{outcomes.insert(QStringLiteral("installationRequested"),true);const auto installed=engine().install(queue,sink(),stop);rebootRequired=rebootRequired || installed.rebootRequired;int failure=installed.cancelled || !installed.error.empty();for(const auto &item:installed.items)failure=failure || item.outcome!=sdiomodern::Outcome::Success;snapshot=sdiomodern::Snapshot();selected.clear();if(failure)return 1;}
            }
            else if(n==QStringLiteral("restorepoint")){engine().createRestorePoint(joined.toStdWString());}
            else if(n==QStringLiteral("checkupdates"))checkUpdates();
            else if(n==QStringLiteral("activetorrent")){activeTorrent=first.toInt();updatesReady=false;}
            else if(n==QStringLiteral("torrentport"))transfer.port=first.toInt();
            else if(n==QStringLiteral("get")){if(first==QStringLiteral("everything")){download(QStringLiteral("app"),QString());download(QStringLiteral("indexes"),QString());download(QStringLiteral("driverpacks"),QStringLiteral("all"));}else download(first,command.arguments.value(1).toLower());}
            else if(n==QStringLiteral("enableinstall")){if(!first.isEmpty())enableInstall=first==QStringLiteral("on");}
            else if(n==QStringLiteral("keeptempfiles")){if(!first.isEmpty())paths.keepTempFiles=first==QStringLiteral("on");}
            else if(n==QStringLiteral("logging")){if(!first.isEmpty())paths.noLog=first==QStringLiteral("off");}
            else if(n==QStringLiteral("debug")){if(!first.isEmpty())paths.verbosity=first==QStringLiteral("on")?0x7fffffff:0;}
            else if(n==QStringLiteral("verbose"))paths.verbosity=first.toInt();
            else if(n==QStringLiteral("echo")){fprintf(stdout,"%s\n",joined.toUtf8().constData());fflush(stdout);}
            else if(n==QStringLiteral("cmd")){
#ifdef Q_OS_WIN
                return QProcess::execute(QStringLiteral("cmd.exe"),QStringList()<<QStringLiteral("/d")<<QStringLiteral("/s")<<QStringLiteral("/c")<<command.raw);
#else
                fail(QStringLiteral("Legacy shell commands require Windows."));
#endif
            }
            else if(n==QStringLiteral("pause")){QTextStream input(stdin);input.readLine();}
            else if(n==QStringLiteral("reboot")){if(first!=QStringLiteral("ifneeded") || rebootRequired){
#ifdef Q_OS_WIN
                const QString shutdown=QDir(QString::fromLocal8Bit(qgetenv("SystemRoot"))).filePath(QStringLiteral("System32/shutdown.exe"));return QProcess::execute(shutdown,QStringList()<<QStringLiteral("-r")<<QStringLiteral("-t")<<QStringLiteral("60"));
#else
                fail(QStringLiteral("Restart requires Windows."));
#endif
            }}
            else if(n==QStringLiteral("runlatest")){if(stagedApp.isEmpty() || !QFileInfo(stagedApp).isFile())fail(QStringLiteral("Download and verify the project application before runlatest."));if(!QProcess::startDetached(stagedApp,QStringList()<<QStringLiteral("--restart-after")<<QString::number(QCoreApplication::applicationPid())<<command.arguments))return 1;}
            else fail(QStringLiteral("Unimplemented legacy runtime command: ")+n);
            return 0;
        }catch(const std::exception &error){QJsonObject item;item.insert(QStringLiteral("line"),command.line);item.insert(QStringLiteral("error"),QString::fromUtf8(error.what()));events.append(item);fprintf(stderr,"Legacy line %d: %s\n",command.line,error.what());return 1;}
    }
};
}
int run(int argc,char **argv){
    QCoreApplication application(argc,argv);application.setOrganizationName(QStringLiteral("TarekSadek"));QStringList arguments;
    QJsonObject report;report.insert(QStringLiteral("installationRequested"),false);QString reportPath,script,validatePath,directInf,directHardware,finish,finishReboot,finishUpdate,archive,hardwareQuery;QStringList parameters;bool nogui=false,autoInstall=false,autoUpdate=false,autoClose=false,exportText=false,preserve=false,onlyUpdates=false,justCheck=false;Runtime runtime;int code=0;
#ifdef Q_OS_WIN
    const HRESULT com=CoInitializeEx(0,COINIT_MULTITHREADED);
#endif
    try{
        arguments=expandConfiguration(application.arguments());
        for(int i=1;i<arguments.size();++i){QString raw=arguments[i];if(raw==QStringLiteral("--legacy-report") || raw==QStringLiteral("--validate-script")){if(++i>=arguments.size())fail(QStringLiteral("Missing legacy option value."));if(raw==QStringLiteral("--legacy-report"))reportPath=arguments[i];else validatePath=arguments[i];continue;}
            if(!script.isEmpty()){parameters<<raw;continue;}
            if(raw.startsWith(QStringLiteral("--")))fail(QStringLiteral("Unsupported mixed legacy option: ")+raw);
            if(!raw.startsWith(QLatin1Char('-')) && !raw.startsWith(QLatin1Char('/'))){if(!script.isEmpty())parameters<<raw;else fail(QStringLiteral("Unexpected legacy argument: ")+raw);continue;}
            raw.remove(0,1);const int colon=raw.indexOf(QLatin1Char(':'));const QString name=(colon<0?raw:raw.left(colon)).toLower(),value=colon<0?QString():raw.mid(colon+1);
            if(name==QStringLiteral("script")){script=value;continue;}
            if(name==QStringLiteral("?")){fprintf(stdout,"Legacy scripts: -script:file.txt [parameters]\nValidation: --validate-script file.txt --legacy-report output.json\nHeadless: -nogui -drp_dir:folder -index_dir:folder [-reindex] [-index_hr]\nAutomation: -autoinstall, -autoupdate, -extractdir:folder, -getdevicelist:file\nArchive: -7z x archive.7z -ofolder\n");return 0;}
            if(name==QStringLiteral("7z")){if(arguments.contains(QStringLiteral("--validate-script")))fail(QStringLiteral("Archive execution cannot be mixed with script validation."));QStringList quoted;for(const QString &token:arguments.mid(i+1))quoted<<(token.contains(QLatin1Char(' '))?QLatin1Char('"')+token+QLatin1Char('"'):token);archive=quoted.join(QStringLiteral(" "));break;}
            if(name==QStringLiteral("drp_dir"))runtime.paths.drivers=value.toStdWString();else if(name==QStringLiteral("index_dir"))runtime.paths.indexes=resolveIndexDirectory(value).toStdWString();else if(name==QStringLiteral("log_dir"))runtime.paths.logDirectory=value.toStdWString();else if(name==QStringLiteral("output_dir"))runtime.paths.indexTextDirectory=value.toStdWString();
            else if(name==QStringLiteral("extractdir")){runtime.extraction=value;runtime.extractOnly=true;}
            else if(name==QStringLiteral("reindex"))runtime.paths.forceReindex=true;else if(name==QStringLiteral("index_hr"))exportText=true;
            else if(name==QStringLiteral("filtersp"))runtime.paths.filterServicePacks=true;else if(name==QStringLiteral("failsafe"))runtime.paths.failSafe=true;else if(name==QStringLiteral("delextrainfs"))runtime.paths.deleteExtraInf=true;else if(name==QStringLiteral("dpinstmode"))runtime.paths.dpInst=true;else if(name==QStringLiteral("keeptempfiles"))runtime.paths.keepTempFiles=true;
            else if(name==QStringLiteral("verbose")){bool ok=false;runtime.paths.verbosity=value.toInt(&ok);if(!ok || runtime.paths.verbosity<0)fail(QStringLiteral("Invalid verbosity."));}
            else if(name==QStringLiteral("nologfile"))runtime.paths.noLog=true;else if(name==QStringLiteral("nogui"))nogui=true;else if(name==QStringLiteral("autoinstall"))autoInstall=true;else if(name==QStringLiteral("autoupdate"))autoUpdate=true;else if(name==QStringLiteral("autoclose"))autoClose=true;
            else if(name==QStringLiteral("disableinstall"))runtime.enableInstall=false;else if(name==QStringLiteral("norestorepoint") || name==QStringLiteral("norestorepnt"))runtime.restore=false;else if(name==QStringLiteral("restorepoint"))runtime.restore=true;
            else if(name==QStringLiteral("ls"))runtime.paths.snapshotFile=value.toStdWString();
            else if(name==QStringLiteral("a")){bool ok=false;runtime.paths.targetArchitecture=value.toInt(&ok);if(!ok || (runtime.paths.targetArchitecture!=32 && runtime.paths.targetArchitecture!=64))fail(QStringLiteral("Invalid target architecture."));}
            else if(name==QStringLiteral("v")){bool ok=false;runtime.paths.targetVersion=value.toInt(&ok);if(!ok)fail(QStringLiteral("Invalid target Windows version."));}
            else if(name==QStringLiteral("finish_cmd"))finish=value;else if(name==QStringLiteral("finishrb_cmd"))finishReboot=value;else if(name==QStringLiteral("finish_upd_cmd"))finishUpdate=value;
            else if(name==QStringLiteral("save-installed-id"))runtime.installedIdLog=value.isEmpty()?QDir(QDir::tempPath()).filePath(QStringLiteral("SDI2/InstalledID.txt")):value;
            else if(name==QStringLiteral("hwidinstalled"))hardwareQuery=value;
            else if(name==QStringLiteral("getdevicelist"))runtime.outcomes.insert(QStringLiteral("deviceListTarget"),value);
            else if(name==QStringLiteral("preservecfg"))preserve=true;
            else if(name==QStringLiteral("lang") || name==QStringLiteral("theme") || name==QStringLiteral("scale")){runtime.outcomes.insert(name,value);}
            else if(name==QStringLiteral("install")){if(i+2>=arguments.size())fail(QStringLiteral("-install needs hardware ID then INF path."));directHardware=arguments[++i];directInf=arguments[++i];}
            else if(name==QStringLiteral("nostop"))runtime.noStop=true;
            else if(name==QStringLiteral("keepunpackedindex"))runtime.paths.keepUnpackedIndex=true;
            else if(name==QStringLiteral("onlyupdates"))onlyUpdates=true;
            else if(name==QStringLiteral("checkupdates"))justCheck=true;
            else if(name==QStringLiteral("port") || name==QStringLiteral("minport") || name==QStringLiteral("maxport") || name==QStringLiteral("downlimit") || name==QStringLiteral("uplimit") || name==QStringLiteral("connections") || name==QStringLiteral("filters") || name==QStringLiteral("hintdelay") || name==QStringLiteral("license") || name==QStringLiteral("wndwx") || name==QStringLiteral("wndwy") || name==QStringLiteral("wndsc")){
                bool ok=false;const int number=value.toInt(&ok);if(!ok || number<0)fail(QStringLiteral("Invalid legacy numeric option: ")+name);
                if(name==QStringLiteral("port")){if(number>65535 || (number && number<1024))fail(QStringLiteral("Invalid listening port."));runtime.transfer.port=number;}
                else if(name==QStringLiteral("minport"))runtime.transfer.outgoingMin=number;
                else if(name==QStringLiteral("maxport"))runtime.transfer.outgoingMax=number;
                else if(name==QStringLiteral("downlimit")){if(number>102400)fail(QStringLiteral("Invalid download limit."));runtime.transfer.downloadKiB=number;}
                else if(name==QStringLiteral("uplimit")){if(number>102400)fail(QStringLiteral("Invalid upload limit."));runtime.transfer.uploadKiB=number;}
                else if(name==QStringLiteral("connections")){if(number>200)fail(QStringLiteral("Too many peer connections."));runtime.transfer.connections=number?number:200;}
                else if(name==QStringLiteral("filters")){if(number>8191)fail(QStringLiteral("Invalid legacy filter mask."));runtime.filterMask=number;}
                else runtime.outcomes.insert(name,number);
            }
            else if(name==QStringLiteral("expertmode") || name==QStringLiteral("showdrpnames1") || name==QStringLiteral("showdrpnames2") || name==QStringLiteral("oldstyle") || name==QStringLiteral("novirusalerts") || name==QStringLiteral("torrentalerts"))runtime.outcomes.insert(name,true);
            else if(name==QStringLiteral("data_dir"))runtime.outcomes.insert(name,value);
            else if(name!=QStringLiteral("nosnapshot") && name!=QStringLiteral("nostamp") && name!=QStringLiteral("keepunpackedindex") && name!=QStringLiteral("showconsole"))fail(QStringLiteral("Unknown legacy option: ")+name);
        }
        if(!validatePath.isEmpty())script=validatePath;
        if(!reportPath.isEmpty())nogui=true;
        if(!hardwareQuery.isEmpty() && validatePath.isEmpty()){
            const int equal=hardwareQuery.lastIndexOf(QLatin1Char('='));const QString needle=equal<0?hardwareQuery:hardwareQuery.left(equal);const QString path=equal<0?QDir(QDir::tempPath()).filePath(QStringLiteral("SDI2/InstalledID.txt")):hardwareQuery.mid(equal+1);QFile file(path);bool found=false;if(file.open(QIODevice::ReadOnly) && file.size()<=16*1024*1024){const QString text=QString::fromLocal8Bit(file.readAll());found=text.contains(needle,Qt::CaseSensitive);}code=found?1:0;report.insert(QStringLiteral("hardwareIdFound"),found);report.insert(QStringLiteral("executed"),true);
        }
        else if(!archive.isEmpty() && validatePath.isEmpty()){code=runtime.engine().archiveCommand(archive.toStdWString());report.insert(QStringLiteral("executed"),true);report.insert(QStringLiteral("archiveCommand"),true);}
        else if(!script.isEmpty()){
            runtime.filterMask=-1;
            QFile file(script);if(!file.open(QIODevice::ReadOnly) || file.size()>1024*1024)fail(QStringLiteral("Cannot read the bounded legacy script."));parameters.prepend(QFileInfo(script).absoluteFilePath());const Program program=parse(file.readAll(),parameters);report=describe(program);report.insert(QStringLiteral("installationRequested"),false);
            if(validatePath.isEmpty()){
                code=execute(program,[&runtime](const Command &command){return runtime.invoke(command);});report.insert(QStringLiteral("executed"),true);
            }
        }else{
            if(justCheck){runtime.checkUpdates();report.insert(QStringLiteral("availableFiles"),runtime.torrent.files.size());}
            if(autoUpdate){runtime.download(QStringLiteral("indexes"),QString());runtime.download(QStringLiteral("driverpacks"),onlyUpdates?QStringLiteral("updates"):QStringLiteral("all"));}
            if(!directInf.isEmpty()){
                if(!runtime.enableInstall || !runtime.paths.snapshotFile.empty() || runtime.paths.targetVersion || runtime.paths.targetArchitecture)fail(QStringLiteral("Direct INF installation is disabled for this preview."));
                // Initialize the original architecture state using an empty private
                // library; direct INF installation does not require driver packs.
                QTemporaryDir emptyLibrary;if(!emptyLibrary.isValid())fail(QStringLiteral("Cannot initialize a private INF session."));const auto previousDrivers=runtime.paths.drivers,previousIndexes=runtime.paths.indexes;runtime.paths.drivers=emptyLibrary.path().toStdWString();runtime.paths.indexes=emptyLibrary.path().toStdWString();runtime.scan();runtime.paths.drivers=previousDrivers;runtime.paths.indexes=previousIndexes;
                if(!runtime.installedIdLog.isEmpty()){QDir().mkpath(QFileInfo(runtime.installedIdLog).absolutePath());runtime.engine().setInstalledIdLog(runtime.installedIdLog.toStdWString());}
                report.insert(QStringLiteral("installationRequested"),true);const auto result=runtime.engine().installInf(directInf.toStdWString(),directHardware.toStdWString(),runtime.sink());runtime.rebootRequired=result.rebootRequired;for(const auto &item:result.items)if(item.outcome!=sdiomodern::Outcome::Success)code=1;
            }else{
                const bool scanNeeded=nogui || autoClose || autoInstall || runtime.extractOnly || exportText || runtime.paths.forceReindex || runtime.outcomes.contains(QStringLiteral("deviceListTarget"));
                if(scanNeeded)runtime.scan();if(exportText){const QString folder=QString::fromStdWString(runtime.paths.indexTextDirectory.empty()?runtime.paths.logDirectory:runtime.paths.indexTextDirectory);QDir().mkpath(folder);runtime.engine().exportIndexText(folder.toStdWString());}
                const QString deviceFile=runtime.outcomes.value(QStringLiteral("deviceListTarget")).toString();if(!deviceFile.isEmpty())runtime.engine().writeDeviceList(deviceFile.toStdWString());
                if(autoInstall || runtime.extractOnly){runtime.select(QStringList()<<QStringLiteral("missing")<<QStringLiteral("newer")<<QStringLiteral("better"));Command command;command.name=QStringLiteral("install");code=runtime.invoke(command);}
            }
            if(!finish.isEmpty() || (runtime.rebootRequired && !finishReboot.isEmpty()) || (autoUpdate && !finishUpdate.isEmpty())){
                const QStringList hooks=QStringList()<<finish<<(runtime.rebootRequired?finishReboot:QString())<<(autoUpdate?finishUpdate:QString());for(const QString &hook:hooks)if(!hook.isEmpty()){Command command;command.name=QStringLiteral("cmd");command.raw=hook;if(runtime.invoke(command))code=1;}
            }
            if(!nogui && !autoClose && !autoInstall && !autoUpdate && directInf.isEmpty()){
                QScopedPointer<QSettings> settings(createAppSettings(application.applicationDirPath(),0,false));if(!preserve){settings->setValue(QStringLiteral("driversPath"),QString::fromStdWString(runtime.paths.drivers));settings->setValue(QStringLiteral("indexesPath"),QString::fromStdWString(runtime.paths.indexes));}
                if(!preserve && runtime.outcomes.contains(QStringLiteral("lang")))settings->setValue(QStringLiteral("language"),runtime.outcomes.value(QStringLiteral("lang")).toString());
                if(runtime.outcomes.contains(QStringLiteral("scale"))){bool valid=false;const int old=runtime.outcomes.value(QStringLiteral("scale")).toString().toInt(&valid);if(!valid || old<128 || old>512)fail(QStringLiteral("Invalid legacy UI scale."));if(!preserve)settings->setValue(QStringLiteral("ui/scalePercent"),qRound(old*100./256));}
                if(runtime.outcomes.contains(QStringLiteral("theme"))){const auto theme=Theme::readPack(runtime.outcomes.value(QStringLiteral("theme")).toString());runtime.outcomes.insert(QStringLiteral("themeDocument"),theme);if(!preserve)settings->setValue(QStringLiteral("theme/document"),QJsonDocument(theme).toJson(QJsonDocument::Compact));}settings->sync();
                QJsonObject session=runtime.outcomes;session.insert(QStringLiteral("drivers"),QString::fromStdWString(runtime.paths.drivers));session.insert(QStringLiteral("indexes"),QString::fromStdWString(runtime.paths.indexes));session.insert(QStringLiteral("targetVersion"),runtime.paths.targetVersion);session.insert(QStringLiteral("targetArchitecture"),runtime.paths.targetArchitecture);session.insert(QStringLiteral("disableInstall"),!runtime.enableInstall);session.insert(QStringLiteral("filterServicePacks"),runtime.paths.filterServicePacks);session.insert(QStringLiteral("failSafe"),runtime.paths.failSafe);session.insert(QStringLiteral("deleteExtraInf"),runtime.paths.deleteExtraInf);session.insert(QStringLiteral("dpInst"),runtime.paths.dpInst);session.insert(QStringLiteral("keepTempFiles"),runtime.paths.keepTempFiles);session.insert(QStringLiteral("keepUnpackedIndex"),runtime.paths.keepUnpackedIndex);session.insert(QStringLiteral("noLog"),runtime.paths.noLog);session.insert(QStringLiteral("filters"),runtime.filterMask);
                qputenv("SDIO_LEGACY_SESSION",QJsonDocument(session).toJson(QJsonDocument::Compact));
                if(!QProcess::startDetached(application.applicationFilePath(),QStringList()<<QStringLiteral("--restart-after")<<QString::number(application.applicationPid())<<QStringLiteral("--no-splash")))code=1;
            }
            report.insert(QStringLiteral("executed"),true);report.insert(QStringLiteral("devices"),int(runtime.snapshot.devices.size()));
        }
        report.insert(QStringLiteral("events"),runtime.events);report.insert(QStringLiteral("results"),runtime.outcomes);if(runtime.outcomes.value(QStringLiteral("installationRequested")).toBool())report.insert(QStringLiteral("installationRequested"),true);report.insert(QStringLiteral("rebootRequired"),runtime.rebootRequired);
    }catch(const std::exception &error){report.insert(QStringLiteral("error"),QString::fromUtf8(error.what()));fprintf(stderr,"%s\n",error.what());code=2;}
    report.insert(QStringLiteral("exitCode"),code);try{saveJson(reportPath,report);}catch(const std::exception &error){fprintf(stderr,"%s\n",error.what());code=3;}
    if(reportPath.isEmpty()){fprintf(stdout,"%s\n",QJsonDocument(report).toJson(QJsonDocument::Compact).constData());fflush(stdout);}
    runtime.core.reset();
#ifdef Q_OS_WIN
    if(SUCCEEDED(com))CoUninitialize();
#endif
    return code;
}
}
