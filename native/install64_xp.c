// SPDX-License-Identifier: GPL-3.0-or-later
// Tiny x64 companion for the original installer's HWID/INF protocol.
// Deliberately uses only XP-era Win32 imports, with no C runtime startup.
#define WINVER 0x0502
#define _WIN32_WINNT 0x0502
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <shellapi.h>
#include <newdev.h>

void helperEntry(void)
{
    int count=0;
    WCHAR **args=CommandLineToArgvW(GetCommandLineW(),&count);
    DWORD result=ERROR_INVALID_PARAMETER;
    if(args && count==3){
        BOOL restart=FALSE;
        if(UpdateDriverForPlugAndPlayDevicesW(NULL,args[1],args[2],INSTALLFLAG_FORCE,&restart))
            result=1u|(restart?0x80000000u:0u);
        else{
            result=GetLastError();
            if(!result)result=ERROR_GEN_FAILURE;
        }
    }
    if(args)LocalFree(args);
    ExitProcess(result);
}
