// SPDX-License-Identifier: GPL-3.0-or-later
// In-process adapter to the pinned C++ SDIO Origin baseline. No SDIO subprocess.
// Build this translation unit WITHOUT Qt headers (Origin has names/macros that
// collide with Qt). Upstream algorithm files are not replaced or reimplemented.
#include "origin_api.h"
#include "com_header.h"
#include "common.h"
#include "logging.h"
#include "system.h"
#include "settings.h"
#include "indexing.h"
#include "matcher.h"
#include "manager.h"
#include "enum.h"
#include "main.h"
#include "model.h"
#include "script.h"
#include "install.h"
#include "theme.h"
#include "7zip.h"
#include "cli.h"
#include <windows.h>
#include <setupapi.h>
#include <objbase.h>
#include <aclapi.h>
#include <SRRestorePtAPI.h>
#include <algorithm>
#include <map>
#include <set>
#include <stdexcept>
#include <sstream>

// Added as a friend ONLY to allow reading the existing arrays/instance IDs.
// There is deliberately no alternative implementation of Origin ranking.
class SdioQtAccess {
public:
    static const std::vector<itembar_t>& items(const Manager& m){return m.items_list;}
    static std::vector<itembar_t>& items(Manager& m){return m.items_list;}
    static ofst instance(const Device& d){return d.InstanceId;}
    static std::size_t first(const Devicematch& d){return d.start_matches;}
    static std::size_t count(const Devicematch& d){return d.num_matches;}
};
extern Event *deviceupdate_event;
extern Event *installupdate_event;
extern Manager manager_v[2];
extern Bundle bundle[2];

namespace sdiomodern {
namespace {
static LONG reserved=0;
static bool initialized=false;
std::wstring wide(const wchar_t* s){return s?std::wstring(s):std::wstring();}
std::wstring ansi(const char* s)
{
    if(!s || !*s)return std::wstring();
    // Origin's indexes expose ANSI INF strings. Preserve the native conversion
    // used by Origin instead of unconditionally interpreting them as UTF-8.
    int n=MultiByteToWideChar(CP_ACP,0,s,-1,0,0);
    if(n<1)return std::wstring();
    std::vector<wchar_t> v(static_cast<std::size_t>(n));
    MultiByteToWideChar(CP_ACP,0,s,-1,&v[0],n);return std::wstring(&v[0]);
}
void report(const EventSink& sink,Stage stage,const std::wstring& message,
            Outcome outcome=Outcome::Running,const std::wstring& id=std::wstring(),
            std::uint64_t done=0,std::uint64_t total=0,std::uint32_t error=0)
{
    if(!sink)return;
    Event e;e.stage=stage;e.outcome=outcome;e.message=message;e.deviceId=id;
    e.done=done;e.total=total;e.nativeError=error;
    e.percent=total?static_cast<int>(done*100/total):-1;sink(e);
}
std::wstring absolutePath(const std::wstring& path)
{
    if(path.empty() || path.size()>2000 || path.find_first_of(L"\"\r\n%")!=std::wstring::npos)
        throw std::runtime_error("Invalid path. Choose a normal local directory without quotes or percent signs.");
    std::vector<wchar_t> buffer(32768);
    DWORD n=GetFullPathNameW(path.c_str(),static_cast<DWORD>(buffer.size()),&buffer[0],0);
    if(n==0 || n>=buffer.size())throw std::runtime_error("Cannot resolve a configured path.");
    std::wstring out(&buffer[0]);
    while(out.size()>3 && out.back()==L'\\')out.pop_back();
    // Existing Origin uses fixed-size Win32 buffers: do not imply long-path support.
    if(out.size()>220)throw std::runtime_error("Origin paths must be short; choose a directory such as C:\\SDIO\\drivers.");
    return out;
}
void copyPath(wchar_t* dest,std::size_t capacity,const std::wstring& source)
{
    if(source.size()+1>=capacity)throw std::runtime_error("Path exceeds the original engine buffer.");
    std::copy(source.begin(),source.end(),dest);dest[source.size()]=0;
}
std::vector<std::wstring> multiString(const wchar_t* p)
{
    std::vector<std::wstring> out;
    if(!p)return out;
    std::size_t used=0;
    while(*p && used<32768) {
        std::size_t length=0;while(p[length] && length<32768-used)++length;
        if(length==32768-used)throw std::runtime_error("Invalid multi-string in device data.");
        out.push_back(std::wstring(p,length));p+=length+1;used+=length+1;
    }
    return out;
}
struct FileStamp {
    DWORD hi,lo;FILETIME write;bool available;
    FileStamp():hi(0),lo(0),available(false){write.dwHighDateTime=write.dwLowDateTime=0;}
};
FileStamp fileStamp(const std::wstring& p)
{
    WIN32_FILE_ATTRIBUTE_DATA a;FileStamp s;
    if(GetFileAttributesExW(p.c_str(),GetFileExInfoStandard,&a) && !(a.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)) {
        s.hi=a.nFileSizeHigh;s.lo=a.nFileSizeLow;s.write=a.ftLastWriteTime;s.available=true;
    }
    return s;
}
bool sameStamp(const FileStamp& a,const FileStamp& b)
{
    return a.available && b.available && a.hi==b.hi && a.lo==b.lo &&
        CompareFileTime(&a.write,&b.write)==0;
}
bool administrator()
{
    SID_IDENTIFIER_AUTHORITY authority=SECURITY_NT_AUTHORITY;PSID sid=0;BOOL member=FALSE;
    if(AllocateAndInitializeSid(&authority,2,SECURITY_BUILTIN_DOMAIN_RID,
          DOMAIN_ALIAS_RID_ADMINS,0,0,0,0,0,0,&sid)) {
        CheckTokenMembership(0,sid,&member);FreeSid(sid);
    }
    return member!=FALSE;
}
// UpdateDriverForPlugAndPlayDevices is HWID-wide, not instance-scoped. Check
// ALL present PnP devices immediately before each call, not merely visible rows.
bool uniquelyPresent(const std::wstring& instance,const std::wstring& hardwareId)
{
    HDEVINFO list=SetupDiGetClassDevsW(0,0,0,DIGCF_PRESENT|DIGCF_ALLCLASSES);
    if(list==INVALID_HANDLE_VALUE)return false;
    const std::wstring key=canonicalId(hardwareId),wanted=canonicalId(instance);
    unsigned matchingDevices=0;bool exact=false,complete=true;
    for(DWORD index=0;;++index) {
        SP_DEVINFO_DATA data={};data.cbSize=sizeof(data);
        if(!SetupDiEnumDeviceInfo(list,index,&data)) {
            if(GetLastError()!=ERROR_NO_MORE_ITEMS)complete=false;
            break;
        }
        bool found=false;
        const DWORD properties[]={SPDRP_HARDWAREID,SPDRP_COMPATIBLEIDS};
        for(unsigned pi=0;pi<2;++pi) {
            DWORD required=0,type=0;
            if(!SetupDiGetDeviceRegistryPropertyW(list,&data,properties[pi],&type,0,0,&required)) {
                const DWORD error=GetLastError();
                if(error==ERROR_INVALID_DATA || error==ERROR_FILE_NOT_FOUND)continue;
                if(error!=ERROR_INSUFFICIENT_BUFFER){complete=false;break;}
            }
            if(required>1024*1024 || required%sizeof(wchar_t)){complete=false;break;}
            if(!required)continue;
            std::vector<wchar_t> buffer(required/sizeof(wchar_t)+2,0);
            if(!SetupDiGetDeviceRegistryPropertyW(list,&data,properties[pi],&type,
                reinterpret_cast<PBYTE>(&buffer[0]),required,&required) || type!=REG_MULTI_SZ) {
                complete=false;break;
            }
            const std::vector<std::wstring> ids=multiString(&buffer[0]);
            for(const auto& id:ids)if(canonicalId(id)==key)found=true;
        }
        if(!complete)break;
        if(found) {
            ++matchingDevices;wchar_t id[2048]={};
            if(!SetupDiGetDeviceInstanceIdW(list,&data,id,2048,0)){complete=false;break;}
            if(wanted.empty() || canonicalId(id)==wanted)exact=true;
            if(matchingDevices>1)break;
        }
    }
    SetupDiDestroyDeviceInfoList(list);
    return complete && matchingDevices==1 && exact;
}
bool safePackageName(const std::wstring& name)
{
    if(name.size()<4 || name.size()>160 || name==L"unpacked.7z")return false;
    if(name.find_first_of(L"\\/:\"%*?\r\n")!=std::wstring::npos || name.find(L"..")!=std::wstring::npos)return false;
    for(const wchar_t c:name)if(c<32)return false;
    return canonicalId(name.substr(name.size()-3))==L".7Z";
}
bool packageWithinRoot(const std::wstring& package,const std::wstring& root)
{
    const std::wstring file=canonicalId(absolutePath(package));
    const std::wstring base=canonicalId(root)+L"\\";
    if(file.compare(0,base.size(),base)!=0)return false;
    // Reparse paths could redirect a validated package outside the chosen root.
    std::wstring current=root;
    if(GetFileAttributesW(current.c_str())&FILE_ATTRIBUTE_REPARSE_POINT)return false;
    std::wstring tail=absolutePath(package).substr(base.size());std::size_t pos=0;
    while(pos<tail.size()) {
        const std::size_t next=tail.find(L'\\',pos);
        current+=L"\\"+tail.substr(pos,next==std::wstring::npos?next:next-pos);
        DWORD attrs=GetFileAttributesW(current.c_str());
        if(attrs!=INVALID_FILE_ATTRIBUTES && (attrs&FILE_ATTRIBUTE_REPARSE_POINT))return false;
        if(next==std::wstring::npos)break;pos=next+1;
    }
    return true;
}
void removePrivateTree(const std::wstring& root)
{
    WIN32_FIND_DATAW data;HANDLE f=FindFirstFileW((root+L"\\*").c_str(),&data);
    if(f!=INVALID_HANDLE_VALUE) {
        do {
            if(!wcscmp(data.cFileName,L".") || !wcscmp(data.cFileName,L".."))continue;
            std::wstring child=root+L"\\"+data.cFileName;
            if(data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) {
                if(data.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT)RemoveDirectoryW(child.c_str());
                else removePrivateTree(child);
            } else DeleteFileW(child.c_str());
        }while(FindNextFileW(f,&data));
        FindClose(f);
    }
    RemoveDirectoryW(root.c_str());
}
struct PrivateDirectory {
    std::wstring path;
    bool keep=false;
    PrivateDirectory(){
        wchar_t temp[MAX_PATH]={};if(!GetTempPathW(MAX_PATH,temp))throw std::runtime_error("No temporary directory.");
        GUID id;wchar_t guid[48]={};if(FAILED(CoCreateGuid(&id)))throw std::runtime_error("Cannot create a unique extraction directory.");
        StringFromGUID2(id,guid,48);path=std::wstring(temp)+L"SDIOModern-"+guid;
        if(path.size()>220 || !CreateDirectoryW(path.c_str(),0))throw std::runtime_error("Cannot create a private extraction directory.");
    }
    ~PrivateDirectory(){if(!keep)removePrivateTree(path);}
};
struct ReadLock {
    HANDLE handle;
    explicit ReadLock(const std::wstring& path):handle(CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,
        0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0)){}
    ~ReadLock(){if(handle!=INVALID_HANDLE_VALUE)CloseHandle(handle);}
    bool valid()const{return handle!=INVALID_HANDLE_VALUE;}
    FileStamp stamp()const {
        FileStamp out;BY_HANDLE_FILE_INFORMATION info={};
        if(valid() && GetFileInformationByHandle(handle,&info) && !(info.dwFileAttributes&(FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_REPARSE_POINT))) {
            out.available=true;out.hi=info.nFileSizeHigh;out.lo=info.nFileSizeLow;out.write=info.ftLastWriteTime;
        }
        return out;
    }
};
}

struct OriginCore::Impl {
    bool pathsKeepTemp=false;
    Snapshot snapshot;
    std::uint64_t generation;
    std::map<std::uint64_t,std::size_t> itemByToken;
    std::map<std::uint64_t,FileStamp> stamps;
    std::wstring driversRoot;
    bool ownsSlot;
    bool logOpen;
    Impl():generation(0),ownsSlot(InterlockedCompareExchange(&reserved,1,0)==0),logOpen(false) {
        if(!ownsSlot)throw std::runtime_error("Only one Origin engine session is supported.");
    }
    ~Impl(){if(logOpen)Log.stop();if(ownsSlot)InterlockedExchange(&reserved,0);}
    void initialize(const Paths& paths) {
        if(!initialized) {
            ghInst=GetModuleHandleW(0);
            SYSTEM_INFO sys;GetSystemInfo(&sys);num_cores=static_cast<int>(sys.dwNumberOfProcessors);
            registerall();
            if(!vLang)vLang=CreateVaultLang(language,STR_NM,IDR_LANG);
            if(!deviceupdate_event)deviceupdate_event=CreateEventWr();
            if(!installupdate_event)installupdate_event=CreateEventWr();
            initialized=true;
        }
        Settings.flags=FLAG_NOGUI|FLAG_NOSNAPSHOT|FLAG_PRESERVECFG|
                       FLAG_SCRIPTMODE|COLLECTION_USE_LZMA|FLAG_KEEPTEMPFILES;
        pathsKeepTemp=paths.keepTempFiles;
        if(paths.forceReindex)Settings.flags|=COLLECTION_FORCE_REINDEXING;
        if(paths.filterServicePacks){Settings.flags|=FLAG_FILTERSP;Settings.flags&=~COLLECTION_USE_LZMA;}
        if(paths.failSafe)Settings.flags|=FLAG_FAILSAFE;
        if(paths.deleteExtraInf)Settings.flags|=FLAG_DELEXTRAINFS;
        if(paths.dpInst)Settings.flags|=FLAG_DPINSTMODE;
        if(paths.keepUnpackedIndex)Settings.flags|=FLAG_KEEPUNPACKINDEX;
        if(paths.noLog)Settings.flags|=FLAG_NOLOGFILE;
        if(paths.verbosity>=0)Log.set_verbose(paths.verbosity);
        Settings.statemode=paths.snapshotFile.empty()?STATEMODE_REAL:STATEMODE_EMUL;
        if(!paths.snapshotFile.empty()){
            const std::wstring snapshotPath=absolutePath(paths.snapshotFile);
            if(!fileStamp(snapshotPath).available)throw std::runtime_error("Snapshot file does not exist.");
            copyPath(Settings.state_file,BUFLEN,snapshotPath);
        }
        Settings.filters=1190; // Original default: missing, newer, better, NF missing, best only.
        if(paths.targetArchitecture!=0 && paths.targetArchitecture!=32 && paths.targetArchitecture!=64)
            throw std::runtime_error("Unsupported preview architecture.");
        Settings.virtual_arch_type=paths.targetArchitecture;
        if((paths.targetVersion==52 || paths.targetVersion==110) && paths.targetArchitecture==32)throw std::runtime_error("This preview Windows version requires 64-bit architecture.");
        if(paths.targetVersion==51 && paths.targetArchitecture==64)throw std::runtime_error("Choose Windows XP x64 for a 64-bit XP preview.");
        if(!paths.targetArchitecture && paths.targetVersion==51)Settings.virtual_arch_type=32;
        if(!paths.targetArchitecture && (paths.targetVersion==52 || paths.targetVersion==110))Settings.virtual_arch_type=64;
        Settings.virtual_os_version=0;
        if(paths.targetVersion){
            WinVersions versions;
            const int index=versions.GetVersionIndex(paths.targetVersion,false);
            if(index<0 || versions.GetEntry(index)!=paths.targetVersion)throw std::runtime_error("Unsupported preview Windows version.");
            Settings.virtual_os_version=index+ID_OS_ITEMS;
        }
        const std::wstring drivers=absolutePath(paths.drivers);driversRoot=drivers;
        const std::wstring indexes=absolutePath(paths.indexes);
        if(GetFileAttributesW(drivers.c_str())==INVALID_FILE_ATTRIBUTES || !(GetFileAttributesW(drivers.c_str())&FILE_ATTRIBUTE_DIRECTORY))
            throw std::runtime_error("Driver package directory does not exist. Set the folders before scanning.");
        if(GetFileAttributesW(indexes.c_str())==INVALID_FILE_ATTRIBUTES || !(GetFileAttributesW(indexes.c_str())&FILE_ATTRIBUTE_DIRECTORY))
            throw std::runtime_error("Index directory does not exist. Set the folders before scanning.");
        copyPath(Settings.drp_dir,sizeof(Settings.drp_dir)/sizeof(wchar_t),drivers);
        copyPath(Settings.index_dir,sizeof(Settings.index_dir)/sizeof(wchar_t),indexes);
        copyPath(Settings.output_dir,BUFLEN,absolutePath(paths.indexTextDirectory.empty()?paths.logDirectory:paths.indexTextDirectory));
        if(!logOpen){
            copyPath(Settings.log_dir,BUFLEN,absolutePath(paths.logDirectory));
            Log.start(Settings.log_dir);logOpen=true;
        }
        installmode=MODE_NONE;
    }
};
class RestorePointError : public std::runtime_error {public:DWORD code;RestorePointError(const std::string &message,DWORD value):std::runtime_error(message),code(value){}};
static HRESULT restoreComSecurity(){
    static bool initialized=false;static HRESULT result=S_OK;if(initialized)return result;initialized=true;
    SID_IDENTIFIER_AUTHORITY nt=SECURITY_NT_AUTHORITY;PSID sids[4]={};
    const DWORD rids[3]={SECURITY_LOCAL_SYSTEM_RID,SECURITY_LOCAL_SERVICE_RID,SECURITY_NETWORK_SERVICE_RID};
    EXPLICIT_ACCESSW entries[5]={};PACL acl=0;SECURITY_DESCRIPTOR descriptor={};BYTE user[256]={};DWORD required=0;HANDLE token=0;
    bool ready=AllocateAndInitializeSid(&nt,2,SECURITY_BUILTIN_DOMAIN_RID,DOMAIN_ALIAS_RID_ADMINS,0,0,0,0,0,0,&sids[0])!=FALSE;
    for(int i=0;i<3 && ready;++i)ready=AllocateAndInitializeSid(&nt,1,rids[i],0,0,0,0,0,0,0,&sids[i+1])!=FALSE;
    if(ready){ready=OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&token) && GetTokenInformation(token,TokenUser,user,sizeof(user),&required);if(token)CloseHandle(token);}
    if(ready){for(int i=0;i<5;++i){entries[i].grfAccessPermissions=3;entries[i].grfAccessMode=SET_ACCESS;entries[i].grfInheritance=NO_INHERITANCE;entries[i].Trustee.TrusteeForm=TRUSTEE_IS_SID;entries[i].Trustee.TrusteeType=TRUSTEE_IS_UNKNOWN;entries[i].Trustee.ptstrName=static_cast<LPWSTR>(i<4?sids[i]:reinterpret_cast<TOKEN_USER*>(user)->User.Sid);}
        const DWORD status=SetEntriesInAclW(5,entries,0,&acl);if(status!=ERROR_SUCCESS)SetLastError(status);ready=status==ERROR_SUCCESS && InitializeSecurityDescriptor(&descriptor,SECURITY_DESCRIPTOR_REVISION) && SetSecurityDescriptorOwner(&descriptor,reinterpret_cast<TOKEN_USER*>(user)->User.Sid,FALSE) && SetSecurityDescriptorGroup(&descriptor,sids[0],FALSE) && SetSecurityDescriptorDacl(&descriptor,TRUE,acl,FALSE);
    }
    result=ready?CoInitializeSecurity(&descriptor,-1,0,0,RPC_C_AUTHN_LEVEL_PKT_PRIVACY,RPC_C_IMP_LEVEL_IMPERSONATE,0,EOAC_NONE,0):HRESULT_FROM_WIN32(GetLastError()?GetLastError():ERROR_GEN_FAILURE);
    if(acl)LocalFree(acl);for(PSID sid:sids)if(sid)FreeSid(sid);return result;
}
std::uint32_t initializeRestoreCallbacks(){return static_cast<std::uint32_t>(restoreComSecurity());}
OriginCore::OriginCore():m(new Impl){restoreComSecurity();}
OriginCore::~OriginCore(){}
Capabilities OriginCore::capabilities()const
{
    Capabilities c;c.nativeCore=true;c.matching=true;c.snapshots=true;c.restorePoint=true;
#ifdef SDIO_MODERN_ENABLE_EXPERIMENTAL_INSTALL
    c.localInstall=true;
#endif
    // No torrent/download claim is made by the XP-compatible offline build.
    c.downloads=false;return c;
}
Snapshot OriginCore::scan(const Paths& paths,const EventSink& sink)
{
    m->snapshot=Snapshot();m->itemByToken.clear();m->stamps.clear();++m->generation;
    m->initialize(paths);
    Log.print_con("Restore callback COM security: 0x%08lX\n",static_cast<unsigned long>(restoreComSecurity()));
    report(sink,Stage::Scan,L"Origin: enumerating devices, loading indexes and matching candidates.");
    const DWORD originStart=GetTickCount();
    Script script;script.initEngine(paths.forceReindex?L"reindex":L""); // original synchronous headless pipeline
    {
        std::wostringstream ms;
        ms<<L"Origin index / PnP scan took "<<static_cast<DWORD>(GetTickCount()-originStart)<<L" ms. Preparing matches.";
        report(sink,Stage::Match,ms.str());
    }
    Matcher* matcher=manager_g->matcher;
    if(!matcher || !matcher->getState() || matcher->getDwidmatch_list()==0)throw std::runtime_error("Origin did not return a usable device scan. Check the engine log and Windows permissions.");
    if(!matcher->getCol() || matcher->getCol()->size()<=1)
        throw std::runtime_error("No driver packs were loaded. Check the drivers folder and compatible .bin indexes; hardware inventory alone is not a successful driver match scan.");
    State* state=matcher->getState();const Txt& text=state->textas;
    Snapshot out;out.generation=m->generation;out.capabilities=capabilities();
    if(paths.disableInstall)out.capabilities.localInstall=false;
    int major=0,minor=0,build=0;state->getWinVer(&major,&minor,&build);out.matchedWindowsVersion=major*10+minor;out.matchedArchitecture=state->getArchitecture()?64:32;
    if(!paths.snapshotFile.empty() || paths.targetVersion || paths.targetArchitecture){
        // A saved machine is never an installation target on this computer.
        out.capabilities.localInstall=false;out.capabilities.restorePoint=false;
    }
    out.sourceRevision=L"SDIO-baseline @ 25468bdb37d46a72ab4e8bf037b4e1a28ffec70e (mirror snapshot)";
    const std::vector<itembar_t>& items=SdioQtAccess::items(*manager_g);
    {
        std::wostringstream counts;
        counts<<L"Matcher returned "<<matcher->getHwidmatch_list()<<L" raw matches and "<<items.size()<<L" manager rows from "<<matcher->getCol()->size()<<L" driver packs.";
        report(sink,Stage::Match,counts.str());
    }
    std::map<const Hwidmatch*,std::size_t> itemMap;
    // A package can match hundreds of devices. File attributes and reparse-path
    // checks must not be repeated for each candidate on the same snapshot.
    struct PackageCheck { FileStamp stamp; bool stamped; bool pathChecked; bool withinRoot;
        PackageCheck():stamped(false),pathChecked(false),withinRoot(false){} };
    std::map<std::wstring,PackageCheck> packageChecks;
    const DWORD copyStart=GetTickCount();
    for(std::size_t i=RES_SLOTS;i<items.size();++i)
        if(items[i].hwidmatch && !(items[i].first&2))itemMap[items[i].hwidmatch]=i;
    for(std::size_t di=0;di<matcher->getDwidmatch_list();++di) {
        Devicematch* match=matcher->getDevicematch_i(di);
        if(!match || !match->device)continue;
        ::Device* device=match->device;sdiomodern::Device d;
        d.instanceId=wide(text.getw(SdioQtAccess::instance(*device)));
        d.name=wide(text.getw(device->getFriendlyName()));
        if(d.name.empty())d.name=wide(text.getw(device->getDescr()));
        d.manufacturer=wide(text.getw(device->getMfg()));d.problemCode=device->getProblem();
        d.matchStatus=SdioQtAccess::count(*match)==0?static_cast<std::uint32_t>(match->getStatus()):0;
        wchar_t category[BUFLEN]={};device->getClassDesc(category);d.category=category;
        if(device->getHardwareID())d.hardwareIds=multiString(text.getw(device->getHardwareID()));
        if(device->getCompatibleIDs())d.compatibleIds=multiString(text.getw(device->getCompatibleIDs()));
        const Driver* current=match->driver;d.hasInstalledDriver=current!=0;
        if(current) {
            d.installed.version=wide(text.getw(current->getDriverVersion()));
            d.installed.date=wide(text.getw(current->getDriverDate()));
            d.installed.provider=wide(text.getw(current->getProviderName()));
            d.installed.inf=wide(text.getw(current->getInfPath()));
            d.installed.section=wide(text.getw(current->getInfSection()));
            d.installed.hardwareId=wide(text.getw(current->getMatchingDeviceId()));
        }
        std::set<std::size_t> emitted;
        bool hasRecommended=false; // same first-recommended semantics, linear-time scan
        for(std::size_t k=0;k<SdioQtAccess::count(*match);++k) {
            Hwidmatch* hw=matcher->getHwidmatch_i(SdioQtAccess::first(*match)+k);
            const auto it=itemMap.find(hw);if(it==itemMap.end() || !emitted.insert(it->second).second)continue;
            Candidate c;c.token=(m->generation<<32)|static_cast<std::uint64_t>(it->second+1);
            c.statusBits=static_cast<std::uint32_t>(hw->getStatus());c.rawScore=hw->getScore();
            c.originalOrder=static_cast<int>(k)+1;
            c.valid=hw->isdrivervalid()!=0 && !(c.statusBits&(STATUS_INVALID|STATUS_DUP));
            WStringShort ver,date;hw->getdrp_drvversion()->str_version(ver);hw->getdrp_drvversion()->str_date(date);
            c.version=wide(ver.Get());c.date=wide(date.Get());c.provider=ansi(hw->getdrp_drvfield(Provider));
            c.inf=ansi(hw->getdrp_infpath())+ansi(hw->getdrp_infname());
            char section[BUFLEN]={};hw->getdrp_drvsection(section);c.section=ansi(section);
            c.hardwareId=ansi(hw->getdrp_drvHWID());c.packageName=wide(hw->getdrp_packname());
            // An online index uses _P_ as its cache identity; the archive is DP_.
            if(c.packageName.compare(0,3,L"_P_")==0)c.packageName[0]=L'D';
            c.packagePath=wide(hw->getdrp_packpath())+L"\\"+c.packageName;
            PackageCheck &check=packageChecks[c.packagePath];
            if(!check.stamped){check.stamp=fileStamp(c.packagePath);check.stamped=true;}
            const FileStamp stamp=check.stamp;c.local=stamp.available;
            // Preserve the original validation order; only cache successful
            // within-root checks for the immutable candidate path in this scan.
            bool eligible=c.valid && safeRelativeInfPath(c.inf) && c.inf.size()<220
                && !d.instanceId.empty() && safePackageName(c.packageName);
            if(eligible && !check.pathChecked) {
                check.withinRoot=packageWithinRoot(c.packagePath,m->driversRoot);
                check.pathChecked=true;
            }
            c.valid=eligible && check.withinRoot && c.hardwareId.size()<1024;
            c.recommended=c.valid && items[it->second].isactive && !hasRecommended;
            if(c.recommended)hasRecommended=true;
            std::wostringstream reason;reason<<L"Origin order #"<<c.originalOrder<<L"; raw score 0x"<<std::hex<<c.rawScore
                <<L"; status bits 0x"<<c.statusBits<<L". This is a matching rank, not a security rating.";
            c.reason=reason.str();
            m->itemByToken[c.token]=it->second;m->stamps[c.token]=stamp;d.candidates.push_back(c);
        }
        out.devices.push_back(d);
    }
    // Evaluate all twelve original filter switches with Manager::filter.
    // Cache its answers on the worker so changing filters never blocks Qt or
    // reimplements the engine's interacting better/worse/current rules.
    // Includes best-only, duplicates and invalid visibility; validity is still
    // enforced independently when creating any installation plan.
    for(unsigned mask=0;mask<4096;++mask){
        manager_g->filter(static_cast<int>(mask<<1));
        for(Device &device:out.devices)for(Candidate &candidate:device.candidates){
            const std::size_t index=m->itemByToken.at(candidate.token);
            if(items[index].isactive)candidate.filterVisibility[mask/64]|=std::uint64_t(1)<<(mask%64);
        }
    }
    manager_g->filter(Settings.filters);
    m->snapshot=out;
    Log.save();
    {
        std::wostringstream ms;
        ms<<L"Prepared "<<out.devices.size()<<L" devices ("<<packageChecks.size()
          <<L" unique local package paths) in "<<static_cast<DWORD>(GetTickCount()-copyStart)<<L" ms.";
        report(sink,Stage::Match,ms.str());
    }
    report(sink,Stage::Match,L"Native Origin scan completed. Candidates are copied from the original matcher.",Outcome::Success,
           L"",out.devices.size(),out.devices.size());
    return out;
}

InstallReport OriginCore::install(const InstallPlan& plan,const EventSink& sink,const std::atomic<bool>& stop)
{
    const std::vector<CheckedSelection> checked=validatePlan(m->snapshot,plan);
    InstallReport out;
#ifndef SDIO_MODERN_ENABLE_EXPERIMENTAL_INSTALL
    throw std::runtime_error("This build does not enable experimental native installation.");
#else
    if(!administrator())throw std::runtime_error("Restart the integrated build as Administrator, rescan and confirm the plan.");
    // Validate the WHOLE queue before the first system change.
    for(const auto& s:checked) {
        if(!uniquelyPresent(s.device->instanceId,s.candidate->hardwareId))throw std::runtime_error("A selected device disappeared. Rescan first.");
        if(!sameStamp(m->stamps.at(s.candidate->token),fileStamp(s.candidate->packagePath)))
            throw std::runtime_error("A selected archive changed after scanning. Rescan first.");
        if(m->itemByToken.find(s.candidate->token)==m->itemByToken.end())throw std::runtime_error("Unknown original engine item.");
    }
    if(stop.load()){out.cancelled=true;return out;}
    if(plan.createRestorePoint) {
        report(sink,Stage::RestorePoint,L"Creating a Windows restore point using the Origin system helper.");
        try {createRestorePoint();out.restoreCreated=true;}
        catch(const RestorePointError& error){out.restoreCreated=false;out.restoreFailed=true;out.restoreError=error.code;out.error=ansi(error.what());}
        catch(const std::exception& error){out.restoreCreated=false;out.restoreFailed=true;out.error=ansi(error.what());}
        if(!out.restoreCreated && !plan.continueOnRestoreFailure) {
            out.error=L"Restore point creation failed. No driver installation was started. "+out.error;
            report(sink,Stage::RestorePoint,out.error,Outcome::Failure,L"",0,checked.size(),out.restoreError);++m->generation;m->snapshot=Snapshot();return out;
        }
        report(sink,Stage::RestorePoint,out.restoreCreated?L"Windows accepted the restore-point request.":L"Restore point failed; continuing because the legacy caller explicitly selected -nostop.",out.restoreCreated?Outcome::Success:Outcome::Skipped);
        if(plan.continueOnRestoreFailure)out.error.clear();
    }
    std::uint64_t completed=0;
    for(const auto& selected:checked) {
        Result result;result.deviceId=selected.device->instanceId;result.name=selected.device->name;
        if(stop.load()) {
            result.outcome=Outcome::Cancelled;result.message=L"Not started (stop after current item).";out.cancelled=true;
            out.items.push_back(result);continue;
        }
        ReadLock archive(selected.candidate->packagePath);
        if(!archive.valid() || !sameStamp(m->stamps.at(selected.candidate->token),archive.stamp()) ||
           !uniquelyPresent(selected.device->instanceId,selected.candidate->hardwareId)) {
            result.outcome=Outcome::Failure;result.message=L"Package or device changed; operation was not started.";
            result.nativeError=GetLastError();out.items.push_back(result);++completed;
            report(sink,Stage::Install,result.message,result.outcome,result.deviceId,completed,checked.size(),result.nativeError);continue;
        }
        try {
            PrivateDirectory work;
            work.keep=m->pathsKeepTemp;
            copyPath(extractdir,BUFLEN,work.path);
            std::vector<itembar_t>& items=SdioQtAccess::items(*manager_g);
            const std::size_t index=m->itemByToken.at(selected.candidate->token);
            if(index>=items.size())throw std::runtime_error("Original engine item became invalid.");
            for(auto& item:items)item.checked=0;
            items[index].checked=1;items[index].isactive=1;
            items[SLOT_RESTORE_POINT].checked=0;
            Settings.flags|=FLAG_KEEPTEMPFILES;
            Settings.flags&=~(FLAG_DISABLEINSTALL|FLAG_AUTOINSTALL|FLAG_AUTOCLOSE|FLAG_NOSTOP);
            instflag=INSTALLDRIVERS;ret_global=0;needreboot=0;installmode=MODE_NONE;
            report(sink,Stage::Install,L"Installing the selected Origin candidate; waiting for Windows.",
                   Outcome::Running,result.deviceId,completed,checked.size());
            // Original extraction + installer. Only one validated item is checked.
            // The patch disables automatic clicks on security prompts, log deletion
            // and shell cleanup; PrivateDirectory owns cleanup instead.
            Manager::thread_install(0);
            const int status=items[index].install_status;
            const unsigned raw=static_cast<unsigned>(ret_global);
            const bool ok=(raw&65535u)==1u && ((raw>>16)&255u)==0u &&
                (status==STR_INST_OK || status==STR_INST_REBOOT);
            result.outcome=ok?Outcome::Success:Outcome::Failure;
            result.nativeError=ok?0:static_cast<std::uint32_t>(items[index].val1);
            result.rebootRequired=ok && ((raw&0x40000000u)!=0 || status==STR_INST_REBOOT);
            result.message=ok?L"Windows reported successful installation.":L"Origin reported failure or did not confirm installation.";
            out.rebootRequired=out.rebootRequired || result.rebootRequired;
        } catch(const std::exception& e) {
            result.outcome=Outcome::Failure;result.message=ansi(e.what());
        }
        installmode=MODE_NONE;out.items.push_back(result);++completed;
        report(sink,Stage::Install,result.message,result.outcome,result.deviceId,completed,checked.size(),result.nativeError);
    }
    // A plan is single-use. No stale UI selection can be applied twice.
    ++m->generation;m->snapshot=Snapshot();m->itemByToken.clear();m->stamps.clear();
    report(sink,Stage::Finished,out.cancelled?L"Queue stopped after the current item.":L"Queue finished. Review each result and rescan.",
           out.cancelled?Outcome::Cancelled:
               (std::any_of(out.items.begin(),out.items.end(),[](const Result& r){return r.outcome==Outcome::Failure;})?Outcome::Failure:Outcome::Success),
           L"",completed,checked.size());
    return out;
#endif
}
void OriginCore::saveSnapshot(const std::wstring& file)
{
    if(!manager_g->matcher || !m->snapshot.generation)throw std::runtime_error("Scan before saving a snapshot.");
    const std::wstring path=absolutePath(file);
    const int flags=Settings.flags;
    Settings.flags&=~FLAG_NOSNAPSHOT; // Suppress automatic snapshots, allow this explicit save.
    int result=1;
    try {result=manager_g->matcher->getState()->save(path.c_str());}
    catch(...){Settings.flags=flags;throw;}
    Settings.flags=flags;
    if(result!=0 || !fileStamp(path).available)throw std::runtime_error("Origin failed to save the snapshot.");
}
void OriginCore::exportIndexText(const std::wstring& folder)
{
    if(!m->snapshot.generation || !manager_g->matcher)throw std::runtime_error("Scan before exporting indexes.");
    const std::wstring root=absolutePath(folder);
    const DWORD attrs=GetFileAttributesW(root.c_str());
    if(root.size()>180 || attrs==INVALID_FILE_ATTRIBUTES || !(attrs&FILE_ATTRIBUTE_DIRECTORY) || (attrs&FILE_ATTRIBUTE_REPARSE_POINT))throw std::runtime_error("Choose an existing, regular index export folder.");
    copyPath(Settings.output_dir,BUFLEN,root);
    Collection *collection=manager_g->matcher->getCol();
    if(!collection || !collection->size())throw std::runtime_error("No loaded driver indexes to export.");
    for(auto &pack:*collection->getList()){
        wchar_t path[BUFLEN]={};pack.getindexfilename(Settings.output_dir,L"txt",path);
        HANDLE writable=CreateFileW(path,GENERIC_WRITE,FILE_SHARE_READ,0,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);
        if(writable==INVALID_HANDLE_VALUE)throw std::runtime_error("The index export destination is not writable.");
        CloseHandle(writable);
    }
    collection->print_index_hr();
    for(auto &pack:*collection->getList()){
        wchar_t path[BUFLEN]={};pack.getindexfilename(Settings.output_dir,L"txt",path);
        const FileStamp written=fileStamp(path);
        if(!written.available || (!written.hi && !written.lo))throw std::runtime_error("An index text report could not be completed.");
    }
}
void OriginCore::writeDeviceList(const std::wstring& file)
{
    if(!m->snapshot.generation || !manager_g->matcher)throw std::runtime_error("Scan before exporting the device list.");
    wchar_t path[BUFLEN]={};copyPath(path,BUFLEN,absolutePath(file));
    if(manager_g->matcher->write_device_list(path))throw std::runtime_error("The original device list could not be saved.");
}
void OriginCore::setOperationOptions(const Paths &paths){
    m->pathsKeepTemp=paths.keepTempFiles;
    const int flags[]={FLAG_FAILSAFE,FLAG_DELEXTRAINFS,FLAG_DPINSTMODE};
    const bool enabled[]={paths.failSafe,paths.deleteExtraInf,paths.dpInst};
    for(unsigned i=0;i<3;++i){if(enabled[i])Settings.flags|=flags[i];else Settings.flags&=~flags[i];}
    if(paths.verbosity>=0)Log.set_verbose(paths.verbosity);
    if(paths.noLog){Log.stop();m->logOpen=false;Settings.flags|=FLAG_NOLOGFILE;}
    else if(!m->logOpen && !paths.logDirectory.empty()){Settings.flags&=~FLAG_NOLOGFILE;copyPath(Settings.log_dir,BUFLEN,absolutePath(paths.logDirectory));Log.start(Settings.log_dir);m->logOpen=true;}
}
int OriginCore::archiveCommand(const std::wstring &arguments){
    if(arguments.empty() || arguments.size()>32760)throw std::runtime_error("Invalid native archive arguments.");
    registerall();return Extract7z((L"7za.exe "+arguments).c_str());
}
void OriginCore::setInstalledIdLog(const std::wstring &file){
    const std::wstring path=absolutePath(file);if(path.size()>180)throw std::runtime_error("Installed ID log path is too long.");
    init_CLIParam();Parse_save_installed_id_swith((L"-save-installed-id:"+path).c_str());RUN_CLI();
}
InstallReport OriginCore::installInf(const std::wstring &file,const std::wstring &hardwareId,const EventSink &sink){
#ifndef SDIO_MODERN_ENABLE_EXPERIMENTAL_INSTALL
    throw std::runtime_error("Native installation is disabled in this build.");
#else
    if(!administrator())throw std::runtime_error("Administrator rights are required for INF installation.");
    if(!m->snapshot.generation || !manager_g->matcher || !m->snapshot.capabilities.localInstall)throw std::runtime_error("Initialize a current-machine session before direct INF installation.");
    const std::wstring inf=absolutePath(file);const FileStamp stamp=fileStamp(inf);
    if(inf.size()<4 || canonicalId(inf.substr(inf.size()-4))!=L".INF" || !stamp.available || stamp.hi || stamp.lo>16*1024*1024 || (GetFileAttributesW(inf.c_str())&FILE_ATTRIBUTE_REPARSE_POINT))throw std::runtime_error("Choose a regular, bounded INF file.");
    if(hardwareId.empty() || hardwareId.size()>1800 || !uniquelyPresent(L"",hardwareId))throw std::runtime_error("The supplied hardware ID must identify exactly one present device.");
    ReadLock source(inf);if(!source.valid() || !sameStamp(stamp,source.stamp()))throw std::runtime_error("The INF file changed during preflight.");
    PrivateDirectory work;copyPath(extractdir,BUFLEN,work.path);wchar_t hwid[BUFLEN]={};copyPath(hwid,BUFLEN,hardwareId);int code=0,reboot=0;
    report(sink,Stage::Install,L"Installing the explicitly supplied INF after checking the present hardware ID.");
    driver_install(hwid,inf.c_str(),&code,&reboot);
    InstallReport output;Result result=decodeLegacyInfResult(code,reboot!=0);result.deviceId=hardwareId;result.name=inf;output.items.push_back(result);output.rebootRequired=result.rebootRequired;++m->generation;m->snapshot=Snapshot();m->itemByToken.clear();m->stamps.clear();return output;
#endif
}
void OriginCore::createRestorePoint(const std::wstring &description)
{
    if(!administrator())throw std::runtime_error("Run as Administrator to create a restore point.");
    typedef BOOL (WINAPI *RestoreFn)(PRESTOREPOINTINFOW,PSTATEMGRSTATUS);
    HMODULE library=LoadLibraryW(L"srclient.dll");
    if(!library)throw RestorePointError("System Restore is unavailable on this Windows installation.",GetLastError());
    RestoreFn restore=reinterpret_cast<RestoreFn>(GetProcAddress(library,"SRSetRestorePointW"));
    if(!restore){const DWORD error=GetLastError();FreeLibrary(library);throw RestorePointError("System Restore API is unavailable.",error);}
    RESTOREPOINTINFOW request={};STATEMGRSTATUS status={};
    request.dwEventType=BEGIN_SYSTEM_CHANGE;request.dwRestorePtType=DEVICE_DRIVER_INSTALL;
    copyPath(request.szDescription,sizeof(request.szDescription)/sizeof(wchar_t),description.empty()?L"Snappy Driver Installer - before driver changes":description);
    const bool started=restore(&request,&status)!=FALSE && status.nStatus==ERROR_SUCCESS;
    DWORD failure=started?ERROR_SUCCESS:(status.nStatus?status.nStatus:GetLastError());
    bool ended=false;
    if(started){
        request.dwEventType=END_SYSTEM_CHANGE;request.llSequenceNumber=status.llSequenceNumber;
        ended=restore(&request,&status)!=FALSE && status.nStatus==ERROR_SUCCESS;
        if(!ended)failure=status.nStatus?status.nStatus:GetLastError();
    }
    FreeLibrary(library);
    if(!started || !ended){std::ostringstream message;message<<"Windows restore point failed (error "<<failure<<", COM security 0x"<<std::hex<<static_cast<unsigned long>(restoreComSecurity())<<"). ";if(failure==ERROR_SERVICE_DISABLED)message<<"System Protection is disabled. Open System Protection and enable protection for the system drive.";else if(failure==ERROR_DISK_FULL)message<<"System Restore has insufficient disk space.";else message<<"Check System Protection and the Windows System Restore services.";throw RestorePointError(message.str(),failure);}
}
std::wstring OriginCore::extract(const InstallPlan& plan,const std::wstring& folder,const EventSink& sink,const std::atomic<bool>& stop)
{
    if(!plan.confirmed || !m->snapshot.generation || plan.generation!=m->snapshot.generation || plan.selections.empty())
        throw std::runtime_error("Scan and select local drivers before extracting.");
    const std::wstring root=absolutePath(folder);
    GUID id;wchar_t guid[48]={};
    if(FAILED(CoCreateGuid(&id)))throw std::runtime_error("Cannot create an extraction session.");
    StringFromGUID2(id,guid,48);
    const std::wstring output=root+L"\\SDIO-export-"+guid;
    if(output.size()>180 || !CreateDirectoryW(output.c_str(),0))throw std::runtime_error("Choose a short writable extraction folder.");
    std::size_t completed=0;
    for(const Selection &selection:plan.selections){
        if(stop.load())throw std::runtime_error("Extraction stopped after the current archive. Partial output was kept.");
        const Candidate *candidate=0;
        for(const Device &device:m->snapshot.devices)if(canonicalId(device.instanceId)==canonicalId(selection.deviceInstanceId))
            for(const Candidate &c:device.candidates)if(c.token==selection.candidateToken)candidate=&c;
        if(!candidate || !candidate->valid || !candidate->local || !safeRelativeInfPath(candidate->inf))
            throw std::runtime_error("An extraction selection is stale or unavailable.");
        ReadLock archive(candidate->packagePath);
        if(!archive.valid() || !sameStamp(m->stamps.at(candidate->token),archive.stamp()))
            throw std::runtime_error("Archive changed after scanning. Rescan before extracting.");
        std::wostringstream suffix;suffix<<output<<L"\\driver-"<<completed+1;
        const std::wstring destination=suffix.str();
        if(!CreateDirectoryW(destination.c_str(),0))throw std::runtime_error("Cannot create the driver extraction directory.");
        copyPath(extractdir,BUFLEN,destination);
        const std::size_t slash=candidate->inf.find_last_of(L"\\/");
        const std::wstring member=slash==std::wstring::npos?L"*":candidate->inf.substr(0,slash+1)+L"*";
        const std::wstring command=L"app x -y \""+candidate->packagePath+L"\" -o\""+destination+L"\" \""+member+L"\"";
        report(sink,Stage::Extract,L"Extracting selected local driver files.",Outcome::Running,selection.deviceInstanceId,completed,plan.selections.size());
        // The original archive callback aborts when its current row is unchecked.
        // Bind it to this selected row for extraction only; no installer is called.
        std::vector<itembar_t>& items=SdioQtAccess::items(*manager_g);
        const std::size_t index=m->itemByToken.at(candidate->token);
        const int previousItem=itembar_act;
        items[index].checked=1;itembar_act=static_cast<int>(index);
        int extracted=1;
        try {extracted=Extract7z(command.c_str());}
        catch(...){items[index].checked=0;itembar_act=previousItem;throw;}
        items[index].checked=0;itembar_act=previousItem;
        if(extracted!=0)throw std::runtime_error("Archive extraction failed. Partial output was kept.");
        if(!fileStamp(destination+L"\\"+candidate->inf).available)throw std::runtime_error("Extraction did not produce the selected INF.");
        ++completed;
        report(sink,Stage::Extract,L"Driver files extracted; no driver was installed.",Outcome::Success,selection.deviceInstanceId,completed,plan.selections.size());
    }
    return output;
}
}
