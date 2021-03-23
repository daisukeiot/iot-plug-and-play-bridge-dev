#include "impinj_reader_r700.h"
#include "restapi.h"
#include "utils.h"
#include "pnp_property.h"
#include "pnp_command.h"
#include "pnp_telemetry.h"

/****************************************************************
PnP Bridge
****************************************************************/
int ImpinjReader_TelemetryWorker(
    void* context)
{
    PNPBRIDGE_COMPONENT_HANDLE componentHandle = (PNPBRIDGE_COMPONENT_HANDLE)context;
    PIMPINJ_READER device                      = Get_Device(componentHandle);

    LogInfo("R700 : %s() enter", __FUNCTION__);

    while (true)
    {
        if (device->ShuttingDown)
        {
            goto exit;
        }
        int uSecInit   = clock();
        int uSecTimer  = 0;
        int uSecTarget = 10000;

        while (uSecTimer < uSecTarget)
        {
            if (device->ShuttingDown)
            {
                return IOTHUB_CLIENT_OK;
            }

            ProcessReaderTelemetry(componentHandle);
            ThreadAPI_Sleep(100);
            uSecTimer = clock() - uSecInit;
        }
    }

exit:
    LogInfo("!!!! R700 : %s() exit", __FUNCTION__);
    ThreadAPI_Exit(0);
    return IOTHUB_CLIENT_OK;
}

/****************************************************************
PnP Bridge
****************************************************************/

/****************************************************************
A callback for Property Update (DEVICE_TWIN_UPDATE_COMPLETE)
****************************************************************/
bool ImpinjReader_OnPropertyCompleteCallback(
    PNPBRIDGE_COMPONENT_HANDLE PnpComponentHandle,
    JSON_Value* Payload,
    void* UserContextCallback)
{
    LogInfo("R700 : %s() enter", __FUNCTION__);
    return OnPropertyCompleteCallback(PnpComponentHandle, Payload, UserContextCallback);
}

/****************************************************************
A callback for Property Update (DEVICE_TWIN_UPDATE_PARTIAL)
****************************************************************/
void
ImpinjReader_OnPropertyPatchCallback(
    PNPBRIDGE_COMPONENT_HANDLE PnpComponentHandle,
    const char* PropertyName,
    JSON_Value* PropertyValue,
    int Version,
    void* ClientHandle)
{
    LogInfo("R700 : %s() enter", __FUNCTION__);
    OnPropertyPatchCallback(PnpComponentHandle, PropertyName, PropertyValue, Version, ClientHandle);
    return;
}

/****************************************************************
A callback for Direct Method
****************************************************************/
int ImpinjReader_OnCommandCallback(
    PNPBRIDGE_COMPONENT_HANDLE PnpComponentHandle,
    const char* CommandName,
    JSON_Value* CommandValue,
    unsigned char** CommandResponse,
    size_t* CommandResponseSize)
{
    LogInfo("R700 : %s() enter", __FUNCTION__);

    return OnCommandCallback(PnpComponentHandle, CommandName, CommandValue, CommandResponse, CommandResponseSize);
}

IOTHUB_CLIENT_RESULT
ImpinjReader_StartPnpComponent(
    PNPBRIDGE_ADAPTER_HANDLE AdapterHandle,
    PNPBRIDGE_COMPONENT_HANDLE PnpComponentHandle)
{
    IOTHUB_CLIENT_RESULT result = IOTHUB_CLIENT_OK;
    AZURE_UNREFERENCED_PARAMETER(AdapterHandle);
    JSON_Value* jsonVal_Rest = NULL;
    int httpStatus;

    LogInfo("R700 : %s() enter", __FUNCTION__);

    PIMPINJ_READER device = Get_Device(PnpComponentHandle);

    // Store client handle before starting Pnp component
    device->ClientHandle = PnpComponentHandleGetClientHandle(PnpComponentHandle);
    device->ShuttingDown = false;

    curlStreamSpawnReaderThread(device->curl_stream_session);

    if (ThreadAPI_Create(&device->WorkerHandle, ImpinjReader_TelemetryWorker, PnpComponentHandle) != THREADAPI_OK)
    {
        LogError("ThreadAPI_Create failed");
        return IOTHUB_CLIENT_ERROR;
    }

    // if ((jsonVal_Rest = ImpinjReader_RequestGet(device, READER_STATUS, NULL, &httpStatus)) != NULL)
    // {
    //     LogJsonPretty(jsonVal_Rest, "R700 : Reader Status (HTTP Status %d)", httpStatus);
    //     json_value_free(jsonVal_Rest);
    // }

    return IOTHUB_CLIENT_OK;
}

IOTHUB_CLIENT_RESULT
ImpinjReader_StopPnpComponent(
    PNPBRIDGE_COMPONENT_HANDLE PnpComponentHandle)
{
    LogInfo("R700 : %s() enter", __FUNCTION__);

    PIMPINJ_READER device = Get_Device(PnpComponentHandle);

    if (device)
    {
        int res;
        device->ShuttingDown = true;
        curlStreamStopThread(device->curl_stream_session);

        if (ThreadAPI_Join(device->WorkerHandle, &res) != THREADAPI_OK)
        {
            LogError("R700 : ThreadAPI_Join failed");
        }
    }

    return IOTHUB_CLIENT_OK;
}

IOTHUB_CLIENT_RESULT
ImpinjReader_CreatePnpAdapter(
    const JSON_Object* AdapterGlobalConfig,
    PNPBRIDGE_ADAPTER_HANDLE AdapterHandle)
{
    AZURE_UNREFERENCED_PARAMETER(AdapterGlobalConfig);
    AZURE_UNREFERENCED_PARAMETER(AdapterHandle);
    LogInfo("R700 : %s() enter", __FUNCTION__);

    curl_global_init(CURL_GLOBAL_DEFAULT);   // initialize cURL globally

    return IOTHUB_CLIENT_OK;
}

IOTHUB_CLIENT_RESULT
ImpinjReader_DestroyPnpAdapter(
    PNPBRIDGE_ADAPTER_HANDLE AdapterHandle)
{
    AZURE_UNREFERENCED_PARAMETER(AdapterHandle);
    LogInfo("R700 : %s() enter", __FUNCTION__);

    curl_global_cleanup();   // cleanup cURL globally

    return IOTHUB_CLIENT_OK;
}

IOTHUB_CLIENT_RESULT
ImpinjReader_CreatePnpComponent(
    PNPBRIDGE_ADAPTER_HANDLE AdapterHandle,
    const char* ComponentName,
    const JSON_Object* AdapterComponentConfig,
    PNPBRIDGE_COMPONENT_HANDLE BridgeComponentHandle)
{
    AZURE_UNREFERENCED_PARAMETER(AdapterHandle);

    IOTHUB_CLIENT_RESULT result = IOTHUB_CLIENT_ERROR;
    PIMPINJ_READER device       = NULL;
    const char* readerHostName  = NULL;
    const char* readerUserName  = NULL;
    const char* readerPassword  = NULL;
    char* http_baseUrl          = NULL;

    LogInfo("R700 : %s() enter", __FUNCTION__);

    /* print component creation message */

    if (strlen(ComponentName) > PNP_MAXIMUM_COMPONENT_LENGTH)
    {
        LogError("R700 : ComponentName=%s is too long.  Maximum length is=%d", ComponentName, PNP_MAXIMUM_COMPONENT_LENGTH);
    }
    else if ((device = calloc(1, sizeof(IMPINJ_READER))) == NULL)
    {
        LogError("R700 : Unable to allocate memory for Impinj Reader component.");
    }
    else if ((device->SensorState = calloc(1, sizeof(IMPINJ_READER_STATE))) == NULL)
    {
        LogError("R700 : Unable to allocate memory for Impinj Reader Sensor State.");
    }
    else if ((readerHostName = json_object_get_string(AdapterComponentConfig, "hostname")) == NULL)
    {
        LogError("R700 : Unable to retrieve Reader Host Name.");
    }
    else
    {
        char build_str_url_always[128] = {0};

        if ((readerUserName = (char*)json_object_get_string(AdapterComponentConfig, "username")) == NULL)
        {
            LogInfo("R700 : Warning : Reader User Name not set");
        }

        if ((readerPassword = (char*)json_object_get_string(AdapterComponentConfig, "password")) == NULL)
        {
            LogInfo("R700 : Warning : Reader password not set");
        }

        LogInfo("R700 : %sComponent Name=%s\r\nHost Name=%s\r\nUser Name=%s%s", g_separator, ComponentName, readerHostName, readerUserName ? readerUserName : "No user name specified", g_separator);

        device->ComponentName = ComponentName;
        mallocAndStrcpy_s(&device->SensorState->componentName, ComponentName);

        sprintf(build_str_url_always, "https://%s/api/v1", readerHostName);
        mallocAndStrcpy_s(&http_baseUrl, build_str_url_always);

        /* initialize cURL sessions */
        CURL_Static_Session_Data* curl_static_session = curlStaticInit(readerUserName, readerPassword, http_baseUrl, VERIFY_CERTS_OFF, VERBOSE_OUTPUT_OFF);
        device->curl_static_session                   = curl_static_session;

        CURL_Stream_Session_Data* curl_stream_session = curlStreamInit(readerUserName, readerPassword, http_baseUrl, VERIFY_CERTS_OFF, VERBOSE_OUTPUT_OFF);
        device->curl_stream_session                   = curl_stream_session;

        PnpComponentHandleSetContext(BridgeComponentHandle, device);
        PnpComponentHandleSetCommandCallback(BridgeComponentHandle, ImpinjReader_OnCommandCallback);
        PnpComponentHandleSetPropertyPatchCallback(BridgeComponentHandle, ImpinjReader_OnPropertyPatchCallback);
        PnpComponentHandleSetPropertyCompleteCallback(BridgeComponentHandle, ImpinjReader_OnPropertyCompleteCallback);
        result = IOTHUB_CLIENT_OK;
    }

    if (result != IOTHUB_CLIENT_OK)
    {
        // if we return non-success, ImpinjReader_DestroyPnpComponent() will not be called.
        if (device != NULL)
        {
            if (device->SensorState != NULL)
            {
                if (device->SensorState->customerName != NULL)
                {
                    free(device->SensorState->customerName);
                }
                free(device->SensorState);
            }

            if (device->curl_stream_session != NULL)
            {
                curlStreamCleanup(device->curl_stream_session);
            }

            if (device->curl_static_session != NULL)
            {
                curlStaticCleanup(device->curl_static_session);
            }

            PnpComponentHandleSetContext(BridgeComponentHandle, NULL);
            free(device);
        }
    }

    return result;
}

IOTHUB_CLIENT_RESULT
ImpinjReader_DestroyPnpComponent(
    PNPBRIDGE_COMPONENT_HANDLE PnpComponentHandle)
{
    PIMPINJ_READER device = NULL;
    LogInfo("R700 : %s() enter", __FUNCTION__);

    device = Get_Device(PnpComponentHandle);

    if (device != NULL)
    {
        if (device->SensorState != NULL)
        {
            if (device->SensorState->customerName != NULL)
            {
                free(device->SensorState->customerName);
            }
            free(device->SensorState);
        }

        if (device->curl_stream_session != NULL)
        {
            curlStreamCleanup(device->curl_stream_session);
        }

        if (device->curl_static_session != NULL)
        {
            curlStaticCleanup(device->curl_static_session);
        }

        PnpComponentHandleSetContext(PnpComponentHandle, NULL);

        free(device);
    }

    return IOTHUB_CLIENT_OK;
}

PNP_ADAPTER ImpinjReaderR700 = {
    .identity            = "impinj-reader-r700",
    .createAdapter       = ImpinjReader_CreatePnpAdapter,
    .createPnpComponent  = ImpinjReader_CreatePnpComponent,
    .startPnpComponent   = ImpinjReader_StartPnpComponent,
    .stopPnpComponent    = ImpinjReader_StopPnpComponent,
    .destroyPnpComponent = ImpinjReader_DestroyPnpComponent,
    .destroyAdapter      = ImpinjReader_DestroyPnpAdapter};