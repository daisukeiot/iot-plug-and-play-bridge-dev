// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif
#include <stdbool.h>
#include <stdarg.h>
#include <assert.h>
#include <parson.h>
#include "azure_c_shared_utility/xlogging.h"

void LogJsonPretty(JSON_Value *Json, const char *MsgFormat, ...);
#ifdef __cplusplus
}
#endif