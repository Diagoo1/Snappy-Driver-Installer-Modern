// SPDX-License-Identifier: GPL-3.0-or-later
// An explicit UI-only developer build. This MUST NOT silently pretend to match.
#include "origin_api.h"
#include <stdexcept>
namespace sdiomodern {
struct OriginCore::Impl {};
OriginCore::OriginCore():m(new Impl){}
OriginCore::~OriginCore(){}
Capabilities OriginCore::capabilities()const{return Capabilities();}
Snapshot OriginCore::scan(const Paths&,const EventSink&){throw std::runtime_error("Native Origin is NOT linked in this executable. Use BUILD_NATIVE_ENGINE.cmd, not the old UI preview build.");}
InstallReport OriginCore::install(const InstallPlan&,const EventSink&,const std::atomic<bool>&){throw std::runtime_error("No native installer is linked. Nothing was installed.");}
void OriginCore::saveSnapshot(const std::wstring&){throw std::runtime_error("No native snapshot engine is linked.");}
std::wstring OriginCore::extract(const InstallPlan&,const std::wstring&,const EventSink&,const std::atomic<bool>&){throw std::runtime_error("No native extraction engine is linked.");}
void OriginCore::createRestorePoint(const std::wstring&){throw std::runtime_error("No native restore-point engine is linked.");}
void OriginCore::exportIndexText(const std::wstring&){throw std::runtime_error("No native index engine is linked.");}
void OriginCore::writeDeviceList(const std::wstring&){throw std::runtime_error("No native device engine is linked.");}
InstallReport OriginCore::installInf(const std::wstring&,const std::wstring&,const EventSink&){throw std::runtime_error("No native INF installer is linked.");}
void OriginCore::setOperationOptions(const Paths&){throw std::runtime_error("No native engine options are linked.");}
int OriginCore::archiveCommand(const std::wstring&){throw std::runtime_error("No native archive engine is linked.");}
void OriginCore::setInstalledIdLog(const std::wstring&){throw std::runtime_error("No native installer ID log is linked.");}
}
