#include "origin_api.h"
#include <cassert>
#include <iostream>
#include <stdexcept>
using namespace sdiomodern;
static int passed=0;
static void check(bool ok){if(!ok)throw std::runtime_error("Test failed");++passed;}
static Snapshot fixture(){
    Snapshot s;s.generation=4;s.capabilities.nativeCore=true;s.capabilities.matching=true;
    s.capabilities.localInstall=true;s.capabilities.restorePoint=true;
    Device d;d.instanceId=L"PCI\\VEN_1234\\ONE";d.hardwareIds.push_back(L"PCI\\VEN_1234&DEV_5678");
    Candidate c;c.token=42;c.valid=true;c.local=true;c.recommended=true;c.inf=L"chipset\\vendor.inf";
    c.hardwareId=d.hardwareIds[0];d.candidates.push_back(c);s.devices.push_back(d);return s;
}
static InstallPlan plan(){InstallPlan p;p.generation=4;p.confirmed=true;p.selections.push_back({L"pci\\ven_1234\\one",42});return p;}
static bool fails(const Snapshot& s,const InstallPlan& p){try{validatePlan(s,p);return false;}catch(const std::exception&){return true;}}
int main(){
    Snapshot s=fixture();InstallPlan p=plan();check(validatePlan(s,p).size()==1);
    p.confirmed=false;check(fails(s,p));p=plan();p.generation=3;check(fails(s,p));
    p=plan();p.selections.clear();check(fails(s,p));p=plan();p.selections.push_back(p.selections[0]);check(fails(s,p));
    p=plan();p.selections[0].candidateToken=43;check(fails(s,p));p=plan();p.selections[0].deviceInstanceId=L"other";check(fails(s,p));
    p=plan();s=fixture();s.capabilities.localInstall=false;check(fails(s,p));s=fixture();s.capabilities.nativeCore=false;check(fails(s,p));
    s=fixture();s.capabilities.restorePoint=false;check(fails(s,p));p.createRestorePoint=false;check(!fails(s,p));p=plan();
    s=fixture();s.devices[0].candidates[0].valid=false;check(fails(s,p));s=fixture();s.devices[0].candidates[0].statusBits=8;check(fails(s,p));
    s=fixture();s.devices[0].candidates[0].local=false;check(fails(s,p));s=fixture();s.devices[0].candidates[0].recommended=false;check(fails(s,p));
    p.allowAlternatives=true;check(!fails(s,p));p=plan();
    s=fixture();s.devices.push_back(s.devices[0]);check(fails(s,p));s=fixture();Device d=s.devices[0];d.instanceId=L"OTHER";s.devices.push_back(d);check(fails(s,p));
    s=fixture();s.devices[0].candidates[0].hardwareId=L"PCI\\UNKNOWN";check(fails(s,p));
    const wchar_t* bad[]={L"..\\x.inf",L"a\\..\\x.inf",L"C:\\x.inf",L"\\\\server\\x.inf",L"/x.inf",L"x.inf\ninstall",L"a\".inf",L"x.exe",L"a\\\\x.inf",L".\\x.inf",L"a.\\x.inf",L"a \\x.inf",L"CON.inf",L"dir\\NUL.inf",L"COM1\\x.inf",L"LPT9.inf",L"file\x01.inf"};
    for(const auto* q:bad)check(!safeRelativeInfPath(q));
    check(safeRelativeInfPath(L"folder\\intel.INF"));check(safeRelativeInfPath(L"folder/file.inf"));
    std::wstring nul=L"file.inf";nul+=L'\0';check(!safeRelativeInfPath(nul));
    s=fixture();s.devices[0].candidates[0].inf=L"..\\x.inf";check(fails(s,p));
    s=fixture();s.devices[0].candidates.push_back(s.devices[0].candidates[0]);check(fails(s,p));
    s=fixture();s.generation=0;check(fails(s,p));
    s=fixture();s.capabilities.matching=false;check(fails(s,p));
    s=fixture();s.devices[0].candidates[0].token=0;p.selections[0].candidateToken=0;check(fails(s,p));p=plan();
    s=fixture();Device other;other.instanceId=L"different";other.hardwareIds.push_back(L"PCI_OTHER");
    s.devices[0].candidates[0].hardwareId=L"PCI_OTHER";s.devices.push_back(other);check(fails(s,p));
    s=fixture();other=Device();other.instanceId=L"second";other.compatibleIds.push_back(s.devices[0].hardwareIds[0]);s.devices.push_back(other);check(fails(s,p));
    s=fixture();s.devices[0].compatibleIds=s.devices[0].hardwareIds;s.devices[0].hardwareIds.clear();check(!fails(s,p));
    check(!safeRelativeInfPath(std::wstring(2049,L'a')+L".inf"));
    check(safeRelativeInfPath(L"valid file.inf"));
    std::cout<<passed<<" native validation checks passed\n";
}
