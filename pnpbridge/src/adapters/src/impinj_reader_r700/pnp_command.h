// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#pragma once

#ifndef R700_COMMAND_H
#define R700_COMMAND_H

#ifdef __cplusplus
extern "C" {
#endif

#include "utils.h"
#include <parson.h>
#include <pnpadapter_api.h>

static const char g_powerSourceAlreadyConfigure[] = "{\"message\":\"The provided power source was already configured on the reader.\"}";

int OnCommandCallback(
    PNPBRIDGE_COMPONENT_HANDLE PnpComponentHandle,
    const char* CommandName,
    JSON_Value* CommandValue,
    unsigned char** CommandResponse,
    size_t* CommandResponseSize);

#ifdef __cplusplus
}
#endif

#endif /* R700_COMMAND_H */