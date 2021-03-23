#include "utils.h"

/****************************************************************
Helper function

Print outs JSON payload with indent
Works only with JSONObject
****************************************************************/
void LogJsonPretty(
    JSON_Value* JsonValue,
    const char* MsgFormat, ...)
{
    JSON_Value_Type jsonType = JSONError;
    char local_Buffer[128]   = {0};
    char* bufferFormat       = NULL;
    char* bufferJson         = NULL;
    char* bufferPtr;
    size_t json_Size = 0;

    va_list argList;
    int len;
    int written;
    JSON_Object* jsonObj  = NULL;
    JSON_Array* jsonArray = NULL;

    if (JsonValue == NULL)
    {
        //LogInfo("R700 : JsonValue = NULL");
    }
    else if ((jsonType = json_value_get_type(JsonValue)) == JSONError)
    {
        LogInfo("R700 : Unable to retrieve JSON type");
    }
    else if (jsonType == JSONNull)
    {
        LogInfo("R700 : JSON is Null type");
    }
    else if ((jsonType == JSONObject) && (jsonObj = json_value_get_object(JsonValue)) == NULL)
    {
        // Only supports JSONObject
        LogInfo("R700 : Unable to retrieve JSON object");
    }
    else if ((jsonType == JSONArray) && (jsonArray = json_value_get_array(JsonValue)) == NULL)
    {
        // Only supports JSONObject
        LogInfo("R700 : Unable to retrieve JSON array");
    }
    else if ((json_Size = json_serialization_size_pretty(JsonValue)) == 0)
    {
        LogError("R700 : Unable to retrieve buffer size of serialization");
    }
    else if ((bufferJson = json_serialize_to_string_pretty(JsonValue)) == NULL)
    {
        LogError("R700 : Unabled to serialize JSON payload");
    }

    va_start(argList, MsgFormat);
    len = vsnprintf(NULL, 0, MsgFormat, argList);
    va_end(argList);

    if (len < 0)
    {
        LogError("R700 : Unnable to determine buffer size");
    }
    else
    {
        len = len + 1 + json_Size + 1;   // for null and CR

        if (len > sizeof(local_Buffer))
        {
            bufferFormat = malloc(len);
        }
        else
        {
            bufferFormat = &local_Buffer[0];
        }
    }

    va_start(argList, MsgFormat);
    written = vsnprintf(bufferFormat, len, MsgFormat, argList);
    va_end(argList);

    if (written < 0)
    {
        LogError("R700 : Unnable to format with JSON");
        goto exit;
    }

    if (bufferJson != NULL)
    {
        bufferPtr = bufferFormat + written;
        written   = snprintf(bufferPtr, len - written, "\r\n%s", bufferJson);
    }

    LogInfo("%s", bufferFormat);

exit:

    if (bufferFormat != &local_Buffer[0])
    {
        free(bufferFormat);
    }

    if (bufferJson != NULL)
    {
        json_free_serialized_string(bufferJson);
    }

    return;
}

void LogJsonPrettyStr(
    char* JsonString,
    const char* MsgFormat, ...)
{
    JSON_Value* jsonVal_Payload;
    va_list argList;

    if (JsonString == NULL)
    {
        jsonVal_Payload = NULL;
    }
    else if ((jsonVal_Payload = json_parse_string(JsonString)) == NULL)
    {
        LogError("R700 : Failed to allocation JSON Value for log. %s", JsonString);
    }

    va_start(argList, MsgFormat);
    LogJsonPretty(jsonVal_Payload, MsgFormat, argList);
    va_end(argList);

    if (jsonVal_Payload)
    {
        json_value_free(jsonVal_Payload);
    }
}

/****************************************************************
Retrieves PIMPINJ_READER from PnpComponentHandle
****************************************************************/
PIMPINJ_READER
Get_Device(
    PNPBRIDGE_COMPONENT_HANDLE PnpComponentHandle)
{
    PIMPINJ_READER device = NULL;

    assert(PnpComponentHandle);

    if (PnpComponentHandle == NULL)
    {
        LogError("R700 : PnPComponentHandle = NULL");
    }
    else
    {
        device = (PIMPINJ_READER)PnpComponentHandleGetContext(PnpComponentHandle);
    }

    return device;
}

/****************************************************************
Removes specified JSON Array from JSON string
****************************************************************/
JSON_Value*
Remove_JSON_Array(
    char* Payload,
    const char* ArrayName)
{
    JSON_Value* jsonVal = NULL;
    JSON_Object* jsonObj;
    JSON_Array* jsonArray;

    if ((jsonVal = json_parse_string(Payload)) == NULL)
    {
        LogError("R700 : Unable to parse GET Response JSON");
    }
    else if ((jsonObj = json_value_get_object(jsonVal)) == NULL)
    {
        LogError("R700 : Cannot retrieve GET Response JSON object");
    }
    else if ((jsonArray = json_object_get_array(jsonObj, ArrayName)) == NULL)
    {
        LogError("R700 : Cannot retrieve JSON Array '%s'", ArrayName);
    }
    else if (json_object_remove(jsonObj, ArrayName) != JSONSuccess)
    {
        LogError("R700 : Cannot remove JSON Array '%s'", ArrayName);
    }
    else
    {
        return jsonVal;
    }

    if (jsonVal)
    {
        json_value_free(jsonVal);
        jsonVal = NULL;
    }

    return jsonVal;
}

/****************************************************************
Converts specified JSON Array to DTDL Map
****************************************************************/
JSON_Value*
JSONArray2DtdlMap(
    char* Payload,
    const char* ArrayName,
    const char* MapName)
{
    JSON_Value* jsonVal_Array  = NULL;
    JSON_Object* jsonObj_Array = NULL;
    JSON_Array* jsonArray      = NULL;
    JSON_Value* jsonVal_Map    = NULL;
    JSON_Object* jsonObj_Map   = NULL;
    int arrayCount;
    JSON_Value_Type json_Type = JSONError;
    int i;

    LogInfo("R700 : %s() enter Array Name=%s Map Name=%s", __FUNCTION__, ArrayName, MapName);

    if ((jsonVal_Array = json_parse_string(Payload)) == NULL)
    {
        LogError("R700 : Unable to parse Payload to JSON_Value");
    }
    else if ((json_Type = json_value_get_type(jsonVal_Array)) == JSONError)
    {
        LogError("R700 : Unable to determine JSON Value type");
    }
    else if (json_Type == JSONArray)
    {
        if ((jsonArray = json_value_get_array(jsonVal_Array)) == NULL)
        {
            LogError("R700 : Cannot retrieve JSON Array");
        }
        else if ((arrayCount = json_array_get_count(jsonArray)) == 0)
        {
            LogError("R700 : JSON Array empty");
        }
        else if ((jsonVal_Map = json_value_init_object()) == NULL)
        {
            LogError("R700 : Cannot create JSON Value for Profiles Map");
        }
        else if ((jsonObj_Map = json_value_get_object(jsonVal_Map)) == NULL)
        {
            LogError("R700 : Cannot retrieve JSON Object for Profiles Map");
        }
    }
    else if (json_Type == JSONObject)
    {
        assert(ArrayName != NULL);

        if ((jsonObj_Array = json_value_get_object(jsonVal_Array)) == NULL)
        {
            LogError("R700 : Cannot create copy of Array Value");
        }
        else if ((jsonArray = json_object_get_array(jsonObj_Array, ArrayName)) == NULL)
        {
            LogInfo("R700 : Did not find %s Array in payload", ArrayName);
        }
        else if ((jsonVal_Map = json_value_deep_copy(jsonVal_Array)) == NULL)
        {
            LogError("R700 : Cannot create copy of Array Value");
        }
        else if ((jsonObj_Map = json_value_get_object(jsonVal_Map)) == NULL)
        {
            LogError("R700 : Cannot retrieve JSON object for Array");
        }
        else if ((arrayCount = json_array_get_count(jsonArray)) == 0)
        {
            LogError("R700 : JSON Object Array empty");
        }
        else if (json_object_remove(jsonObj_Map, ArrayName) != JSONSuccess)
        {
            LogError("R700 : Failed to remove Array from payload");
        }
    }

    for (i = 0; i < arrayCount; i++)
    {
        char buffer[32] = {0};
        if (ArrayName)
        {
            JSON_Value* jsonVal_ArrayValue     = NULL;
            JSON_Value* jsonVal_ArrayValueCopy = NULL;
            sprintf(buffer, "%s.%s%d", ArrayName, MapName, i);

            if ((jsonVal_ArrayValue = json_array_get_value(jsonArray, i)) == NULL)
            {
                LogError("R700 : Unable to retrieve JSON Value from Object Array %s", ArrayName);
                break;
            }
            else if ((jsonVal_ArrayValueCopy = json_value_deep_copy(jsonVal_ArrayValue)) == NULL)
            {
                LogError("R700 : Cannot create a copy of bootstrap Value");
                break;
            }
            else if ((json_object_dotset_value(jsonObj_Map, buffer, jsonVal_ArrayValueCopy)) != JSONSuccess)
            {
                LogError("R700 : Unable to retrieve JSON Value from Object Array %s", ArrayName);
                break;
            }
        }
        else
        {
            const char* profilevalue = json_array_get_string(jsonArray, i);
            sprintf(buffer, "%s%d", MapName, i);
            json_object_set_string(jsonObj_Map, buffer, profilevalue);
        }
    }

    if (jsonVal_Array)
    {
        json_value_free(jsonVal_Array);
    }

    return jsonVal_Map;
}

/****************************************************************
Converts DTDL Map to JSON Array
****************************************************************/
char* DtdlMap2JSONArray(
    const char* Payload,
    const char* ArrayName)
{
    JSON_Value* jsonVal_Payload = NULL;
    JSON_Object* jsonObj_Payload;
    JSON_Value* jsonVal_Map;
    JSON_Object* jsonObj_Map;
    int mapCount;
    JSON_Value* jsonVal_Array = NULL;
    JSON_Object* jsonObj_Array;
    JSON_Value* jsonVal_ArrayValue;
    JSON_Array* jsonArray_ArrayValue;
    bool bSuccess    = false;
    char* arrayValue = NULL;

    LogInfo("R700 : %s() enter Array Name=%s", __FUNCTION__, ArrayName);
    LogJsonPrettyStr((char*)Payload, "Map Payload");

    if ((jsonVal_Payload = json_parse_string(Payload)) == NULL)
    {
        LogError("R700 : Unable to parse Map Payload to JSON_Value");
    }
    else if ((jsonObj_Payload = json_value_get_object(jsonVal_Payload)) == NULL)
    {
        LogError("R700 : Cannot retrieve JSON object for Kafka payload");
    }
    else if (json_object_has_value(jsonObj_Payload, ArrayName) == 0)
    {
        LogInfo("R700 : DTDL Map %s not found in  payload", ArrayName);
    }
    else if ((jsonVal_Map = json_object_get_value(jsonObj_Payload, ArrayName)) == NULL)
    {
        LogError("R700 : Cannnot retrieve JSON Value for %s", ArrayName);
    }
    else if (json_value_get_type(jsonVal_Map) != JSONObject)
    {
        LogError("R700 : Map %s is not JSON Object", ArrayName);
    }
    else if ((jsonObj_Map = json_value_get_object(jsonVal_Map)) == NULL)
    {
        LogError("R700 : Cannnot retrieve JSON Object for Kafka Bootstraps");
    }
    else if ((mapCount = json_object_get_count(jsonObj_Map)) == 0)
    {
        LogInfo("R700 : Map %s empty", ArrayName);
    }
    else if ((jsonVal_Array = json_value_deep_copy(jsonVal_Payload)) == NULL)
    {
        LogError("R700 : Cannot create copy of Kafka JSON Value");
    }
    else if ((jsonObj_Array = json_value_get_object(jsonVal_Array)) == NULL)
    {
        LogError("R700 : Cannnot retrieve JSON Object for Kafka Bootstraps copy");
    }
    else if (json_object_remove(jsonObj_Array, ArrayName) != JSONSuccess)
    {
        LogError("R700 : Failed to remove %s", ArrayName);
    }
    else if ((jsonVal_ArrayValue = json_value_init_array()) == NULL)
    {
        LogError("R700 : Failed to create JSON Value for %s array", ArrayName);
    }
    else if ((jsonArray_ArrayValue = json_value_get_array(jsonVal_ArrayValue)) == NULL)
    {
        LogError("R700 : Failed to retrieve %s array", ArrayName);
    }
    else
    {
        int i;

        for (i = 0; i < mapCount; i++)
        {
            JSON_Value* jsonVal_MapValue;
            JSON_Value* jsonVal_MapValueCopy;

            if ((jsonVal_MapValue = json_object_get_value_at(jsonObj_Map, i)) == NULL)
            {
                LogError("R700 : Unable to retrieve %s index %d", ArrayName, i);
                goto exit;
            }
            else if ((jsonVal_MapValueCopy = json_value_deep_copy(jsonVal_MapValue)) == NULL)
            {
                LogError("R700 : Unable to append bootstrap value index %d", i);
                goto exit;
            }
            else if ((json_array_append_value(jsonArray_ArrayValue, jsonVal_MapValueCopy)) != JSONSuccess)
            {
                LogError("R700 : Unable to append bootstrap value index %d", i);
                goto exit;
            }
        }

        json_object_set_value(jsonObj_Array, ArrayName, json_array_get_wrapping_value(jsonArray_ArrayValue));
        arrayValue = json_serialize_to_string(jsonVal_Array);
        LogJsonPretty(jsonVal_Array, "R700 : Array Payload");
    }

exit:

    if (jsonVal_Payload)
    {
        json_value_free(jsonVal_Payload);
    }

    if (jsonVal_Array)
    {
        json_value_free(jsonVal_Array);
    }

    return arrayValue;
}

const char*
GetStringFromPayload(
    JSON_Value* Payload,
    const char* ParamName)
{
    JSON_Object* jsonObj_Payload;
    JSON_Value* jsonVal_Payload;
    const char* commandValue = NULL;

    JSON_Value* jsonVal_PresetObject    = NULL;
    JSON_Array* jsonArray_AntennaConfig = NULL;

    if ((jsonObj_Payload = json_value_get_object(Payload)) == NULL)
    {
        LogError("R700 : Unable to retrieve command payload JSON object");
    }
    else if ((jsonVal_Payload = json_object_get_value(jsonObj_Payload, ParamName)) == NULL)
    {
        LogError("R700 : Unable to retrieve command payload JSON Value for %s.", ParamName);
    }
    else if ((commandValue = json_value_get_string(jsonVal_Payload)) == NULL)
    {
        LogError("R700 : Unable to retrieve %s.", ParamName);
    }

    return commandValue;
}