#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include <string.h>
#include "utils.h"
#include "impinj_reader_r700.h"
#include "azure_c_shared_utility/xlogging.h"
#include "azure_c_shared_utility/crt_abstractions.h"
#include "azure_c_shared_utility/const_defines.h"

/****************************************************************
PnP Bridge
****************************************************************/

/****************************************************************
A callback for Property Update (DEVICE_TWIN_UPDATE_COMPLETE)
****************************************************************/
bool ImpinjReader_OnPropertyCompleteCallback(
    PNPBRIDGE_COMPONENT_HANDLE PnpComponentHandle,
    JSON_Value *Payload,
    void *userContextCallback)
{
    LogJsonPretty(Payload, "R700 : %s()", __FUNCTION__);

    return true;
}

/****************************************************************
A callback for Property Update (DEVICE_TWIN_UPDATE_PARTIAL)
****************************************************************/
void
ImpinjReader_OnPropertyPatchCallback(
    PNPBRIDGE_COMPONENT_HANDLE PnpComponentHandle,
    const char *PropertyName,
    JSON_Value *PropertyValue,
    int version,
    void *ClientHandle)
{
    LogJsonPretty(PropertyValue, "R700 : %s() Property=%s", __FUNCTION__, PropertyName);

    return;
}

int
ImpinjReader_OnCommandCallback(
    PNPBRIDGE_COMPONENT_HANDLE PnpComponentHandle,
    const char *CommandName,
    JSON_Value *CommandValue,
    unsigned char **CommandResponse,
    size_t *CommandResponseSize)
{
    LogJsonPretty(CommandValue, "R700 : %s() Command %s", __FUNCTION__, CommandName);
    
    return PNP_STATUS_SUCCESS;
}

IOTHUB_CLIENT_RESULT
ImpinjReader_StartPnpComponent(
    PNPBRIDGE_ADAPTER_HANDLE AdapterHandle,
    PNPBRIDGE_COMPONENT_HANDLE PnpComponentHandle)
{
    IOTHUB_CLIENT_RESULT result = IOTHUB_CLIENT_OK;
    AZURE_UNREFERENCED_PARAMETER(AdapterHandle);

    LogInfo("R700 : %s enter", __FUNCTION__);

    return IOTHUB_CLIENT_OK;
}

IOTHUB_CLIENT_RESULT
ImpinjReader_StopPnpComponent(
    PNPBRIDGE_COMPONENT_HANDLE PnpComponentHandle)
{
    LogInfo("R700 : %s enter", __FUNCTION__);

    return IOTHUB_CLIENT_OK;
}

IOTHUB_CLIENT_RESULT
ImpinjReader_DestroyPnpComponent(
    PNPBRIDGE_COMPONENT_HANDLE PnpComponentHandle)
{
    LogInfo("R700 : %s enter", __FUNCTION__);

    return IOTHUB_CLIENT_OK;
}

IOTHUB_CLIENT_RESULT
ImpinjReader_DestroyPnpAdapter(
    PNPBRIDGE_ADAPTER_HANDLE AdapterHandle)
{
    AZURE_UNREFERENCED_PARAMETER(AdapterHandle);
    LogInfo("R700 : %s enter", __FUNCTION__);

    return IOTHUB_CLIENT_OK;
}

IOTHUB_CLIENT_RESULT
ImpinjReader_CreatePnpAdapter(
    const JSON_Object *AdapterGlobalConfig,
    PNPBRIDGE_ADAPTER_HANDLE AdapterHandle)
{
    AZURE_UNREFERENCED_PARAMETER(AdapterGlobalConfig);
    AZURE_UNREFERENCED_PARAMETER(AdapterHandle);
    LogInfo("R700 : %s enter", __FUNCTION__);

    return IOTHUB_CLIENT_OK;
}

IOTHUB_CLIENT_RESULT
ImpinjReader_CreatePnpComponent(
    PNPBRIDGE_ADAPTER_HANDLE AdapterHandle,
    const char *ComponentName,
    const JSON_Object *AdapterComponentConfig,
    PNPBRIDGE_COMPONENT_HANDLE BridgeComponentHandle)
{
    AZURE_UNREFERENCED_PARAMETER(AdapterComponentConfig);
    AZURE_UNREFERENCED_PARAMETER(AdapterHandle);

    IOTHUB_CLIENT_RESULT result = IOTHUB_CLIENT_ERROR;
    PIMPINJ_READER device = NULL;

    LogInfo("R700 : %s enter", __FUNCTION__);

    /* print component creation message */

    if (strlen(ComponentName) > PNP_MAXIMUM_COMPONENT_LENGTH)
    {
        LogError("R700 : ComponentName=%s is too long.  Maximum length is=%d", ComponentName, PNP_MAXIMUM_COMPONENT_LENGTH);
    }
    else if ((device = calloc(1, sizeof(IMPINJ_READER))) == NULL)
    {
        LogError("Unable to allocate memory for Impinj Reader component.");
    }
    else
    {
        device->ComponentName = ComponentName;
        PnpComponentHandleSetContext(BridgeComponentHandle, device);
        PnpComponentHandleSetCommandCallback(BridgeComponentHandle, ImpinjReader_OnCommandCallback);
        PnpComponentHandleSetPropertyPatchCallback(BridgeComponentHandle, ImpinjReader_OnPropertyPatchCallback);
        PnpComponentHandleSetPropertyCompleteCallback(BridgeComponentHandle, ImpinjReader_OnPropertyCompleteCallback);
        result = IOTHUB_CLIENT_OK;
    }

    if (result != IOTHUB_CLIENT_OK)
    {

    }

    return result;
}

PNP_ADAPTER ImpinjReaderR700 = {
    .identity = "impinj-reader-r700",
    .createAdapter = ImpinjReader_CreatePnpAdapter,
    .createPnpComponent = ImpinjReader_CreatePnpComponent,
    .startPnpComponent = ImpinjReader_StartPnpComponent,
    .stopPnpComponent = ImpinjReader_StopPnpComponent,
    .destroyPnpComponent = ImpinjReader_DestroyPnpComponent,
    .destroyAdapter = ImpinjReader_DestroyPnpAdapter
    };