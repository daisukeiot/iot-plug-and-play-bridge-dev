#include "pnp_property.h"

/****************************************************************
GetDesiredJson retrieves JSON_Object* in the JSON tree
corresponding to the desired payload.
****************************************************************/
static JSON_Object*
GetDesiredJson(
    DEVICE_TWIN_UPDATE_STATE updateState,
    JSON_Value* rootValue)
{
    JSON_Object* rootObject = NULL;
    JSON_Object* desiredObject;

    if ((rootObject = json_value_get_object(rootValue)) == NULL)
    {
        LogError("Unable to get root object of JSON");
        desiredObject = NULL;
    }
    else
    {
        if (updateState == DEVICE_TWIN_UPDATE_COMPLETE)
        {
            // For a complete update, the JSON from IoTHub will contain both "desired" and "reported" - the full twin.
            // We only care about "desired" in this sample, so just retrieve it.
            desiredObject = json_object_get_object(rootObject, g_IoTHubTwinDesiredObjectName);
        }
        else
        {
            // For a patch update, IoTHub does not explicitly put a "desired:" JSON envelope.  The "desired-ness" is implicit
            // in this case, so here we simply need the root of the JSON itself.
            desiredObject = rootObject;
        }
    }

    return desiredObject;
}

/****************************************************************
A callback for Property Update (DEVICE_TWIN_UPDATE_COMPLETE)
****************************************************************/
bool OnPropertyCompleteCallback(
    PNPBRIDGE_COMPONENT_HANDLE PnpComponentHandle,
    JSON_Value* JsonVal_Payload,
    void* UserContextCallback)
{
    PIMPINJ_READER device = Get_Device(PnpComponentHandle);
    bool bRet             = false;
    JSON_Object* jsonObj_Desired;
    JSON_Value* jsonVal_Version;
    int version;

    LogJsonPretty(JsonVal_Payload, "R700 : %s() enter", __FUNCTION__);

    if ((jsonObj_Desired = GetDesiredJson(DEVICE_TWIN_UPDATE_COMPLETE, JsonVal_Payload)) == NULL)
    {
        LogError("R700 : Unable to  retrieve desired JSON object");
    }
    else if ((jsonVal_Version = json_object_get_value(jsonObj_Desired, g_IoTHubTwinDesiredVersion)) == NULL)
    {
        LogError("R700 : Unable to retrieve %s for desired property", g_IoTHubTwinDesiredVersion);
    }
    else if (json_value_get_type(jsonVal_Version) != JSONNumber)
    {
        // The $version must be a number (and in practice an int) A non-numerical value indicates
        // something is fundamentally wrong with how we've received the twin and we should not proceed.
        LogError("R700 : %s is not JSONNumber", g_IoTHubTwinDesiredVersion);
    }
    else if ((version = (int)json_value_get_number(jsonVal_Version)) == 0)
    {
        LogError("R700 : Unable to retrieve %s", g_IoTHubTwinDesiredVersion);
    }
    else
    {
        JSON_Object* jsonObj_Desired_Component = NULL;
        JSON_Object* jsonObj_Desired_Property  = NULL;
        int i;
        JSON_Value* jsonVal_Rest            = NULL;
        JSON_Value* jsonValue_Desired_Value = NULL;
        int status                          = PNP_STATUS_SUCCESS;

        // Device Twin may not have properties for this component right after provisioning
        if ((jsonObj_Desired_Component = json_object_get_object(jsonObj_Desired, device->ComponentName)) == NULL)
        {
            LogInfo("R700 : Unable to retrieve desired JSON Object for Component %s", device->ComponentName);
        }

        for (i = 0; i < (sizeof(R700_REST_LIST) / sizeof(IMPINJ_R700_REST)); i++)
        {
            jsonVal_Rest            = NULL;
            jsonValue_Desired_Value = NULL;

            switch (R700_REST_LIST[i].DtdlType)
            {
                case COMMAND:
                    break;

                case READONLY:
                    jsonVal_Rest = ImpinjReader_RequestGet(device, i, NULL, &status);
                    UpdateReadOnlyReportProperty(PnpComponentHandle, device->ComponentName, R700_REST_LIST[i].Name, jsonVal_Rest);
                    break;

                case WRITABLE:
                    if ((jsonObj_Desired_Component == NULL) ||
                        ((jsonObj_Desired_Property = json_object_get_object(jsonObj_Desired_Component, R700_REST_LIST[i].Name)) == NULL))
                    {
                        // No desired property for this property.  Send Reported Property with version = 1 with current values from reader
                        jsonVal_Rest = ImpinjReader_RequestGet(device, i, NULL, &status);
                        UpdateWritableProperty(PnpComponentHandle, &R700_REST_LIST[i], jsonVal_Rest, status, "Value from Reader", 1);
                    }
                    else
                    {
                        // Desired Property found for this property.
                        jsonValue_Desired_Value = json_object_get_value(jsonObj_Desired_Component, R700_REST_LIST[i].Name);
                        OnPropertyPatchCallback(PnpComponentHandle, R700_REST_LIST[i].Name, jsonValue_Desired_Value, version, NULL);
                    }

                    break;
            }
        }
        bRet = true;
    }

    return bRet;
}

/****************************************************************
A callback for Property Update (DEVICE_TWIN_UPDATE_PARTIAL)
****************************************************************/
void OnPropertyPatchCallback(
    PNPBRIDGE_COMPONENT_HANDLE PnpComponentHandle,
    const char* PropertyName,
    JSON_Value* JsonVal_Property,
    int Version,
    void* ClientHandle)
{
    PIMPINJ_READER device = Get_Device(PnpComponentHandle);
    int i;
    R700_REST_REQUEST r700_api = UNSUPPORTED;
    JSON_Value* jsonVal_Rest   = NULL;
    char* payload              = NULL;
    int httpStatus             = PNP_STATUS_SUCCESS;
    LogJsonPretty(JsonVal_Property, "R700 : %s() enter", __FUNCTION__);

    for (int i = 0; i < (sizeof(R700_REST_LIST) / sizeof(IMPINJ_R700_REST)); i++)
    {
        if (R700_REST_LIST[i].DtdlType != WRITABLE)
        {
            continue;
        }
        else if (strcmp(PropertyName, R700_REST_LIST[i].Name) == 0)
        {
            LogInfo("R700 : Found property %s i = %d req=%d", R700_REST_LIST[i].Name, i, R700_REST_LIST[i].Requst);
            r700_api = i;
            break;
        }
    }

    if (r700_api != UNSUPPORTED)
    {
        payload = json_serialize_to_string(JsonVal_Property);

        if (R700_REST_LIST[r700_api].RestType == PUT)
        {
            jsonVal_Rest = ImpinjReader_RequestPut(device, r700_api, NULL, payload, &httpStatus);
        }
        else if (R700_REST_LIST[r700_api].RestType == POST)
        {
            jsonVal_Rest = ImpinjReader_RequestPost(device, r700_api, payload, &httpStatus);
        }
        else if (R700_REST_LIST[r700_api].RestType == DELETE)
        {
            jsonVal_Rest = ImpinjReader_RequestDelete(device, r700_api, payload, &httpStatus);
        }
        else
        {
            LogError("R700 : Unknown Request %d", r700_api);
            assert(false);
        }

        UpdateWritableProperty(PnpComponentHandle,
                               &R700_REST_LIST[r700_api],
                               jsonVal_Rest,
                               httpStatus,
                               NULL,
                               Version);

        if (payload)
        {
            json_free_serialized_string(payload);
        }

        if (jsonVal_Rest)
        {
            json_value_free(jsonVal_Rest);
        }
    }

    return;
}

/****************************************************************
A callback from IoT Hub on Reported Property Update
****************************************************************/
static void
ReportedPropertyCallback(
    int ReportedStatus,
    void* UserContextCallback)
{
    LogInfo("R700 : PropertyCallback called, result=%d, property name=%s",
            ReportedStatus,
            (const char*)UserContextCallback);
}

/****************************************************************
Processes Read Only Property Update
****************************************************************/
IOTHUB_CLIENT_RESULT
UpdateReadOnlyReportProperty(
    PNPBRIDGE_COMPONENT_HANDLE PnpComponentHandle,
    const char* ComponentName,
    char* PropertyName,
    JSON_Value* JsonVal_Property)
{
    IOTHUB_CLIENT_RESULT iothubClientResult = IOTHUB_CLIENT_OK;
    STRING_HANDLE jsonToSend                = NULL;
    char* propertyValue                     = NULL;

    LogJsonPretty(JsonVal_Property, "R700 : %s() enter", __FUNCTION__);

    if ((propertyValue = json_serialize_to_string(JsonVal_Property)) == NULL)
    {
        LogError("R700 : Unabled to serialize Read Only Property JSON payload");
    }
    else if ((jsonToSend = PnP_CreateReportedProperty(ComponentName, PropertyName, propertyValue)) == NULL)
    {
        LogError("R700 : Unable to build reported property response for Read Only Property %s, Value=%s",
                 PropertyName,
                 propertyValue);
    }
    else
    {
        PNP_BRIDGE_CLIENT_HANDLE clientHandle = PnpComponentHandleGetClientHandle(PnpComponentHandle);

        if ((iothubClientResult = PnpBridgeClient_SendReportedState(clientHandle,
                                                                    STRING_c_str(jsonToSend),
                                                                    STRING_length(jsonToSend),
                                                                    ReportedPropertyCallback,
                                                                    (void*)PropertyName)) != IOTHUB_CLIENT_OK)
        {
            LogError("R700 : Unable to send reported state for property=%s, error=%d",
                     PropertyName,
                     iothubClientResult);
        }
        else
        {
            LogInfo("R700 : Send Read Only Property %s to IoT Hub", PropertyName);
        }

        STRING_delete(jsonToSend);
    }

    if (propertyValue)
    {
        json_free_serialized_string(propertyValue);
    }

    return iothubClientResult;
}

/****************************************************************
Processes Writable Property
****************************************************************/
IOTHUB_CLIENT_RESULT
UpdateWritableProperty(
    PNPBRIDGE_COMPONENT_HANDLE PnpComponentHandle,
    PIMPINJ_R700_REST RestRequest,
    JSON_Value* JsonVal_Property,
    int Ac,
    char* Ad,
    int Av)
{
    PIMPINJ_READER device                   = Get_Device(PnpComponentHandle);
    IOTHUB_CLIENT_RESULT iothubClientResult = IOTHUB_CLIENT_OK;
    STRING_HANDLE jsonToSend                = NULL;
    char* propertyName                      = RestRequest->Name;
    int httpStatus;
    JSON_Object* jsonObj_Property = NULL;
    JSON_Value* jsonVal_Message   = NULL;
    JSON_Value* jsonVal_Rest      = NULL;
    const char* descToSend        = Ad;
    char* propertyValue           = NULL;

    LogJsonPretty(JsonVal_Property, "R700 : %s() enter AC=%d AV=%d AD=%s", __FUNCTION__, Ac, Av, Ad);

    // Check status (Ack Code)
    switch (Ac)
    {
        case R700_STATUS_NO_CONTENT:
            // Reader did not return any content
            // Read the current/new settings from HW
            assert(JsonVal_Property == NULL);
            jsonVal_Rest = ImpinjReader_RequestGet(device, RestRequest->Requst, NULL, &httpStatus);
            descToSend   = ImpinjReader_ProcessResponse(RestRequest, JsonVal_Property, Ac);
            break;

        case R700_STATUS_ACCEPTED:
            // Reader responded with some context
            assert(JsonVal_Property != NULL);
            jsonVal_Rest = ImpinjReader_RequestGet(device, RestRequest->Requst, NULL, &httpStatus);
            descToSend   = ImpinjReader_ProcessResponse(RestRequest, JsonVal_Property, Ac);
            break;

        case R700_STATUS_BAD_REQUEST:
        case R700_STATUS_FORBIDDEN:
        case R700_STATUS_NOT_FOUND:
        case R700_STATUS_NOT_CONFLICT:
        case R700_STATUS_INTERNAL_ERROR:
            // Errors
            assert(JsonVal_Property != NULL);
            jsonVal_Rest = ImpinjReader_RequestGet(device, RestRequest->Requst, NULL, &httpStatus);
            descToSend   = ImpinjReader_ProcessErrorResponse(JsonVal_Property, RestRequest->DtdlType);
            break;
    }

    if (jsonVal_Rest)
    {
        propertyValue = json_serialize_to_string(jsonVal_Rest);
    }
    else
    {
        propertyValue = json_serialize_to_string(JsonVal_Property);
    }

    if ((jsonToSend = PnP_CreateReportedPropertyWithStatus(device->SensorState->componentName,
                                                           propertyName,
                                                           propertyValue,
                                                           Ac,
                                                           descToSend,
                                                           Av)) == NULL)
    {
        LogError("R700 : Unable to build reported property response for Writable Property %s, Value=%s",
                 propertyName,
                 propertyValue);
    }
    else
    {
        PNP_BRIDGE_CLIENT_HANDLE clientHandle = PnpComponentHandleGetClientHandle(PnpComponentHandle);

        LogJsonPrettyStr((char*)STRING_c_str(jsonToSend), "R700 : Sending Reported Property Json");

        if ((iothubClientResult = PnpBridgeClient_SendReportedState(clientHandle,
                                                                    STRING_c_str(jsonToSend),
                                                                    STRING_length(jsonToSend),
                                                                    ReportedPropertyCallback,
                                                                    (void*)propertyName)) != IOTHUB_CLIENT_OK)
        {
            LogError("R700 : Unable to send reported state for Writable Property %s :: error=%d",
                     propertyName,
                     iothubClientResult);
        }
        else
        {
            LogInfo("R700 : Sent reported state for Writable Property %s", propertyName);
        }
    }

    // clean up
    if (jsonToSend)
    {
        STRING_delete(jsonToSend);
    }

    if (jsonVal_Rest)
    {
        json_value_free(jsonVal_Rest);
    }

    if (propertyValue)
    {
        json_free_serialized_string(propertyValue);
    }

    return iothubClientResult;
}