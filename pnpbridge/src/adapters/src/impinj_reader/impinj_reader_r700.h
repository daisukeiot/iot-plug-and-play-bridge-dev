// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <pnpadapter_api.h>
#include <parson.h>
#include "utils.h"

typedef struct _IMPINJ_READER
{
    const char *ComponentName;
} IMPINJ_READER, *PIMPINJ_READER;


#ifdef __cplusplus
}
#endif