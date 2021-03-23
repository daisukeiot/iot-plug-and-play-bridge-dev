// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <pnpbridge.h>
#include <unistd.h>
#include "azure_c_shared_utility/xlogging.h"

#ifdef WIN32
#include <windows.h>

BOOL WINAPI CtrlHandler(DWORD fdwCtrlType)
{
    switch (fdwCtrlType)
    {
        // Handle the CTRL-C signal. 
    case CTRL_C_EVENT:
        PnpBridge_Stop();
        return TRUE;

    default:
        return FALSE;
        //
    }
}
#else 
#include <signal.h>
void CtrlHandler(int s) {
    PnpBridge_Stop();
}
#endif

int main(int argc, char *argv[])
{
    LogInfo("\n -- Press Ctrl+C to stop PnpBridge\n");

#ifdef WIN32
    SetConsoleCtrlHandler(CtrlHandler, TRUE);
#else
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = CtrlHandler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;

    sigaction(SIGINT, &sigIntHandler, NULL);
#endif

    char* ConfigurationFilePath = NULL;
    if (argc > 1)
    {
        LogInfo("R700 : Using configuration from specified file path: %s", argv[1]);
        mallocAndStrcpy_s(&ConfigurationFilePath, argv[1]);
    }
    else
    {
        if ( access("config.json", F_OK) != 0)
        {
            LogError("R700 : config.json not found");
            return 1;
        }
        LogInfo("R700 : Using default configuration location");
        mallocAndStrcpy_s(&ConfigurationFilePath, (const char*) "config.json");
    }

    PnpBridge_Main((const char*)ConfigurationFilePath);

    return 0;
}
