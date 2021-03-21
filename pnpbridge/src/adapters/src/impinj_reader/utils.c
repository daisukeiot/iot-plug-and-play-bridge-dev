#include "utils.h"

/****************************************************************
Helper function

Print outs JSON payload with indent
Works only with JSONObject
****************************************************************/
void LogJsonPretty(JSON_Value *Json, const char *MsgFormat, ...)
{
    char local_Buffer[128] = {0};
    char *bufferFormat = NULL;
    char *bufferJson = NULL;
    char *bufferPtr;
    size_t json_Size;

    va_list argList;
    int len;
    int written;
    JSON_Object *jsonObj;

    if (Json == NULL)
    {
        return;
    }
    else if ((jsonObj = json_value_get_object(Json)) == NULL)
    {
        // Only supports JSONObject
        return;
    }
    else if (json_object_get_count(jsonObj) == 0)
    {
        // Emptry object.  Nothing to print
        return;
    }
    else if ((json_Size = json_serialization_size_pretty(Json)) == 0)
    {
        LogError("R700 : Unable to retrieve buffer size of serialization");
    }
    else if ((bufferJson = json_serialize_to_string_pretty(Json)) == NULL)
    {
        LogError("R700 : Unabled to serialize JSON payload");
    }
    else
    {
        va_start(argList, MsgFormat);
        len = vsnprintf(NULL, 0, MsgFormat, argList);
        va_end(argList);

        if (len < 0)
        {
            LogError("R700 : Unnable to determine buffer size");
        }
        else
        {
            len = len + 1 + json_Size + 1; // for null and CR

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
    }

    assert(bufferFormat != NULL);
    bufferPtr = bufferFormat + written;
    written = snprintf(bufferPtr, len - written, "\r\n%s", bufferJson);

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

