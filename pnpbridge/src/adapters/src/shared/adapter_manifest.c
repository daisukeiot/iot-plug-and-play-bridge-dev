// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

//Pnp Adapter headers
#include <pnpadapter_api.h>

#ifndef IMPINJ_BUILD
extern PNP_ADAPTER SerialPnpInterface;
extern PNP_ADAPTER ModbusPnpInterface;
extern PNP_ADAPTER MqttPnpInterface;
extern PNP_ADAPTER VirtualEnvironmentSensorSample;

#ifdef WIN32

extern PNP_ADAPTER BluetoothSensorPnpInterface;
extern PNP_ADAPTER CameraPnpInterface;
extern PNP_ADAPTER CoreDeviceHealth;

PPNP_ADAPTER PNP_ADAPTER_MANIFEST[] = {
    &CameraPnpInterface,
    &CoreDeviceHealth,
    &BluetoothSensorPnpInterface,
    &ModbusPnpInterface,
    &MqttPnpInterface,
    &SerialPnpInterface,
    &VirtualEnvironmentSensorSample
};

#else //WIN32

PPNP_ADAPTER PNP_ADAPTER_MANIFEST[] = {
    &ModbusPnpInterface,
    &MqttPnpInterface,
    &SerialPnpInterface,
    &VirtualEnvironmentSensorSample
};

#endif
#else //IMPINJ_BUILD
extern PNP_ADAPTER ImpinjReaderR700;

PPNP_ADAPTER PNP_ADAPTER_MANIFEST[] = {
    &ImpinjReaderR700
};
#endif //IMPINJ_BUILD

const int PnpAdapterCount = sizeof(PNP_ADAPTER_MANIFEST) / sizeof(PPNP_ADAPTER);

