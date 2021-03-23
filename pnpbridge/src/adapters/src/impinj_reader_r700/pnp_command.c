#include "pnp_command.h"
#include "restapi.h"


/****************************************************************
Creates command response payload
****************************************************************/
int
ImpinjReader_SetCommandResponse(
    unsigned char** CommandResponse,
    size_t* CommandResponseSize,
    const unsigned char* ResponseData)
{
    int result = PNP_STATUS_SUCCESS;

    if (ResponseData == NULL)
    {
        LogError("Impinj Reader Adapter:: Response Data is empty");
        *CommandResponseSize = 0;
        return PNP_STATUS_INTERNAL_ERROR;
    }

    *CommandResponseSize = strlen((char*)ResponseData);
    memset(CommandResponse, 0, sizeof(*CommandResponse));

    // Allocate a copy of the response data to return to the invoker. Caller will free this.
    if (mallocAndStrcpy_s((char**)CommandResponse, (char*)ResponseData) != 0)
    {
        LogError("Impinj Reader Adapter:: Unable to allocate response data");
        result = PNP_STATUS_INTERNAL_ERROR;
    }

    return result;
}

/****************************************************************
A callback for Direct Method
****************************************************************/
int
OnCommandCallback(
    PNPBRIDGE_COMPONENT_HANDLE PnpComponentHandle,
    const char* CommandName,
    JSON_Value* CommandValue,
    unsigned char** CommandResponse,
    size_t* CommandResponseSize)
{
    PIMPINJ_READER device = Get_Device(PnpComponentHandle);
    int i;
    int httpStatus                   = PNP_STATUS_INTERNAL_ERROR;
    char* restBody                   = NULL;
    JSON_Value* jsonVal_RestResponse = NULL;
    char* restResponse               = NULL;
    char* restParameter              = NULL;
    R700_REST_REQUEST r700_api       = UNSUPPORTED;
    const char* responseString       = NULL;

    LogJsonPretty(CommandValue, "R700 : %s() enter.  Command Name=%s", __FUNCTION__, CommandName);

    for (i = 0; i < (sizeof(R700_REST_LIST) / sizeof(IMPINJ_R700_REST)); i++)
    {
        if (R700_REST_LIST[i].DtdlType != COMMAND)
        {
            continue;
        }
        else if (strcmp(CommandName, R700_REST_LIST[i].Name) == 0)
        {
            LogInfo("R700 : Found Command %s : Property %s", CommandName, R700_REST_LIST[i].Name);

            r700_api = R700_REST_LIST[i].Requst;

            switch (R700_REST_LIST[r700_api].Requst)
            {

                case PROFILES_INVENTORY_PRESETS_ID_SET:
                    // Receive Preset ID
                    restParameter = (char*)GetStringFromPayload(CommandValue, g_presetId);
                    break;

                case PROFILES_INVENTORY_PRESETS_ID_GET:
                case PROFILES_INVENTORY_PRESETS_ID_DELETE:
                case PROFILES_START: {
                    // Receive Preset ID
                    restParameter = (char*)GetStringFromPayload(CommandValue, g_presetId);
                    break;
                }

                case PROFILES_INVENTORY_TAG:
                    break;

                case SYSTEM_POWER_SET:
                case SYSTEM_TIME_SET:
                    // takes JSON payload for REST API Body
                    restBody = json_serialize_to_string(CommandValue);
                    break;
            }

            // Call REST API
            switch (R700_REST_LIST[r700_api].RestType)
            {
                case GET:
                    jsonVal_RestResponse = ImpinjReader_RequestGet(device, R700_REST_LIST[r700_api].Requst, restParameter, &httpStatus);
                    break;
                case PUT:
                    jsonVal_RestResponse = ImpinjReader_RequestPut(device, R700_REST_LIST[r700_api].Requst, restParameter, restBody, &httpStatus);
                    break;
                case POST:
                    jsonVal_RestResponse = ImpinjReader_RequestPost(device, R700_REST_LIST[r700_api].Requst, restParameter, &httpStatus);
                    break;
                case DELETE:
                    jsonVal_RestResponse = ImpinjReader_RequestDelete(device, R700_REST_LIST[r700_api].Requst, restParameter, &httpStatus);
                    break;
            }
        }
    }

    if (r700_api != UNSUPPORTED)
    {
        LogJsonPretty(jsonVal_RestResponse, "R700 : Command Rest Response");

        switch (httpStatus)
        {
            case R700_STATUS_OK:
            case R700_STATUS_CREATED:
                restResponse = json_serialize_to_string(jsonVal_RestResponse);
                break;

            case R700_STATUS_ACCEPTED:
                restResponse = (char*)ImpinjReader_ProcessResponse(&R700_REST_LIST[r700_api], jsonVal_RestResponse, httpStatus);
                LogInfo("R700 : Command Response \"%s\"", restResponse);
                break;

            case R700_STATUS_NO_CONTENT:
                break;
            case R700_STATUS_BAD_REQUEST:
            case R700_STATUS_FORBIDDEN:
            case R700_STATUS_NOT_FOUND:
            case R700_STATUS_NOT_CONFLICT:
            case R700_STATUS_INTERNAL_ERROR:
                restResponse = ImpinjReader_ProcessErrorResponse(jsonVal_RestResponse, R700_REST_LIST[r700_api].DtdlType);
                break;
        }
    }

    if (restResponse == NULL)
    {
        if (r700_api == SYSTEM_POWER_SET)
        {
            responseString = &g_powerSourceAlreadyConfigure[0];
        }
        else
        {
            responseString = &g_emptyCommandResponse[0];
        }
    }
    else
    {
        responseString = restResponse;
    }

    if (ImpinjReader_SetCommandResponse(CommandResponse, CommandResponseSize, responseString) != PNP_STATUS_SUCCESS)
    {
        httpStatus = PNP_STATUS_INTERNAL_ERROR;
    }

    // clean up
    if (restBody)
    {
        json_free_serialized_string(restBody);
    }

    if (jsonVal_RestResponse)
    {
        json_value_free(jsonVal_RestResponse);
    }

    if (restResponse)
    {
        free(restResponse);
    }

    return httpStatus;
}
