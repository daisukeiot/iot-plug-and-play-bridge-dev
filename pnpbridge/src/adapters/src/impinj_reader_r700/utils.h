// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

#ifndef R700_UTILS_H
#define R700_UTILS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdarg.h>
#include <assert.h>
#include <parson.h>
#include "azure_c_shared_utility/xlogging.h"
#include "impinj_reader_r700.h"


static const char g_separator[] = "\r\n================================\r\n";

void LogJsonPretty(
    JSON_Value* JsonValue,
    const char* MsgFormat, ...);

void LogJsonPrettyStr(
    char* JsonString,
    const char* MsgFormat, ...);

PIMPINJ_READER
Get_Device(
    PNPBRIDGE_COMPONENT_HANDLE PnpComponentHandle);

JSON_Value*
Remove_JSON_Array(
    char* Payload,
    const char* ArrayName);

JSON_Value*
JSONArray2DtdlMap(
    char* Payload,
    const char* ArrayName,
    const char* MapName);

char* DtdlMap2JSONArray(
    const char* Payload,
    const char* ArrayName);

const char*
GetStringFromPayload(
    JSON_Value* CommandValue,
    const char* ParamName);

#ifdef __cplusplus
}
#endif

#endif /* R700_UTILS_H */