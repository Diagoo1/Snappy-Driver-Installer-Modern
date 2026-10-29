#include "origin_api.h"
#include <atomic>
#include <iostream>
#include <stdexcept>
using namespace sdiomodern;
int main() {
    OriginCore core;Capabilities c=core.capabilities();int checks=0;
    const bool states[]={c.nativeCore,c.matching,c.localInstall,c.restorePoint,c.snapshots,c.downloads};
    for(bool state:states){if(state)throw std::runtime_error("Unavailable backend claims a capability");++checks;}
    std::atomic<bool> stop(false);
    try {core.scan(Paths(),EventSink());throw std::logic_error("Scan unexpectedly succeeded");}
    catch(const std::runtime_error&){++checks;}
    try {core.install(InstallPlan(),EventSink(),stop);throw std::logic_error("Install unexpectedly succeeded");}
    catch(const std::runtime_error&){++checks;}
    try {core.saveSnapshot(L"test.snp");throw std::logic_error("Snapshot unexpectedly succeeded");}
    catch(const std::runtime_error&){++checks;}
    std::cout<<checks<<" unavailable-backend checks passed\n";
}
