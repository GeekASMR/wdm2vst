/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    minipairs.h

Abstract:

    Local audio endpoint filter definitions. 
--*/

#ifndef _VIRTUALAUDIODRIVER_MINIPAIRS_H_
#define _VIRTUALAUDIODRIVER_MINIPAIRS_H_

#include "branding.h"
#include "speakertopo.h"
#include "speakertoptable.h"
#include "speakerwavtable.h"

#include "micarraytopo.h"
#include "micarray1toptable.h"
#include "micarraywavtable.h"


NTSTATUS
CreateMiniportWaveRTVirtualAudioDriver
( 
    _Out_       PUNKNOWN *,
    _In_        REFCLSID,
    _In_opt_    PUNKNOWN,
    _In_        POOL_FLAGS,
    _In_        PUNKNOWN,
    _In_opt_    PVOID,
    _In_        PENDPOINT_MINIPAIR
);

NTSTATUS
CreateMiniportTopologyVirtualAudioDriver
( 
    _Out_       PUNKNOWN *,
    _In_        REFCLSID,
    _In_opt_    PUNKNOWN,
    _In_        POOL_FLAGS,
    _In_        PUNKNOWN,
    _In_opt_    PVOID,
    _In_        PENDPOINT_MINIPAIR
);

//
// Render miniports.
//

/*********************************************************************
* Topology/Wave bridge connection for speaker (internal)             *
*                                                                    *
*              +------+                +------+                      *
*              | Wave |                | Topo |                      *
*              |      |                |      |                      *
* System   --->|0    1|--------------->|0    1|---> Line Out         *
*              |      |                |      |                      *
*              +------+                +------+                      *
*********************************************************************/

// Each Speaker endpoint uses its own topology filter descriptor
// with a unique pin Name GUID for distinct endpoint naming.

static
ENDPOINT_MINIPAIR SpeakerMiniports =
{
    eSpeakerDevice,
    L"TopologySpeaker",
    NULL,
    CreateMiniportTopologyVirtualAudioDriver,
    &SpeakerTopoMiniportFilterDescriptor,
    0, NULL,
    L"WaveSpeaker",
    NULL,
    CreateMiniportWaveRTVirtualAudioDriver,
    &SpeakerWaveMiniportFilterDescriptor,
    0, NULL,
    SPEAKER_DEVICE_MAX_CHANNELS,
    SpeakerPinDeviceFormatsAndModes,
    SIZEOF_ARRAY(SpeakerPinDeviceFormatsAndModes),
    SpeakerTopologyPhysicalConnections,
    SIZEOF_ARRAY(SpeakerTopologyPhysicalConnections),
    ENDPOINT_NO_FLAGS,
    0,      // IpcChannelId = 0
    FALSE,  // IpcIsCapture = FALSE (render)
};

static
ENDPOINT_MINIPAIR SpeakerMiniports2 =
{
    eSpeakerDevice2,
    L"TopologySpeaker2",
    NULL,
    CreateMiniportTopologyVirtualAudioDriver,
    &SpeakerTopoMiniportFilterDescriptor2,
    0, NULL,
    L"WaveSpeaker2",
    NULL,
    CreateMiniportWaveRTVirtualAudioDriver,
    &SpeakerWaveMiniportFilterDescriptor,
    0, NULL,
    SPEAKER_DEVICE_MAX_CHANNELS,
    SpeakerPinDeviceFormatsAndModes,
    SIZEOF_ARRAY(SpeakerPinDeviceFormatsAndModes),
    SpeakerTopologyPhysicalConnections,
    SIZEOF_ARRAY(SpeakerTopologyPhysicalConnections),
    ENDPOINT_NO_FLAGS,
    1,      // IpcChannelId = 1
    FALSE,
};

static
ENDPOINT_MINIPAIR SpeakerMiniports3 =
{
    eSpeakerDevice3,
    L"TopologySpeaker3",
    NULL,
    CreateMiniportTopologyVirtualAudioDriver,
    &SpeakerTopoMiniportFilterDescriptor3,
    0, NULL,
    L"WaveSpeaker3",
    NULL,
    CreateMiniportWaveRTVirtualAudioDriver,
    &SpeakerWaveMiniportFilterDescriptor,
    0, NULL,
    SPEAKER_DEVICE_MAX_CHANNELS,
    SpeakerPinDeviceFormatsAndModes,
    SIZEOF_ARRAY(SpeakerPinDeviceFormatsAndModes),
    SpeakerTopologyPhysicalConnections,
    SIZEOF_ARRAY(SpeakerTopologyPhysicalConnections),
    ENDPOINT_NO_FLAGS,
    2,      // IpcChannelId = 2
    FALSE,
};

static
ENDPOINT_MINIPAIR SpeakerMiniports4 =
{
    eSpeakerDevice4,
    L"TopologySpeaker4",
    NULL,
    CreateMiniportTopologyVirtualAudioDriver,
    &SpeakerTopoMiniportFilterDescriptor4,
    0, NULL,
    L"WaveSpeaker4",
    NULL,
    CreateMiniportWaveRTVirtualAudioDriver,
    &SpeakerWaveMiniportFilterDescriptor,
    0, NULL,
    SPEAKER_DEVICE_MAX_CHANNELS,
    SpeakerPinDeviceFormatsAndModes,
    SIZEOF_ARRAY(SpeakerPinDeviceFormatsAndModes),
    SpeakerTopologyPhysicalConnections,
    SIZEOF_ARRAY(SpeakerTopologyPhysicalConnections),
    ENDPOINT_NO_FLAGS,
    3,      // IpcChannelId = 3
    FALSE,
};

//
// Capture miniports.
//

/*********************************************************************
* Topology/Wave bridge connection for mic array  1 (front)           *
*                                                                    *
*              +------+    +------+                                  *
*              | Topo |    | Wave |                                  *
*              |      |    |      |                                  *
*  Mic in  --->|0    1|===>|0    1|---> Capture Host Pin             *
*              |      |    |      |                                  *
*              +------+    +------+                                  *
*********************************************************************/
static
PHYSICALCONNECTIONTABLE MicArray1TopologyPhysicalConnections[] =
{
    {
        KSPIN_TOPO_BRIDGE,          // TopologyOut
        KSPIN_WAVE_BRIDGE,          // WaveIn
        CONNECTIONTYPE_TOPOLOGY_OUTPUT
    }
};

static
ENDPOINT_MINIPAIR MicArray1Miniports =
{
    eMicArrayDevice1,
    L"TopologyMicArray1",
    NULL,
    CreateMicArrayMiniportTopology,
    &MicArray1TopoMiniportFilterDescriptor,
    0, NULL,
    L"WaveMicArray1",
    NULL,
    CreateMiniportWaveRTVirtualAudioDriver,
    &MicArrayWaveMiniportFilterDescriptor,
    0, NULL,
    MICARRAY_DEVICE_MAX_CHANNELS,
    MicArrayPinDeviceFormatsAndModes,
    SIZEOF_ARRAY(MicArrayPinDeviceFormatsAndModes),
    MicArray1TopologyPhysicalConnections,
    SIZEOF_ARRAY(MicArray1TopologyPhysicalConnections),
    ENDPOINT_NO_FLAGS,
    0,      // IpcChannelId = 0
    TRUE,   // IpcIsCapture = TRUE (capture)
};

static
ENDPOINT_MINIPAIR MicArray2Miniports =
{
    eMicArrayDevice2,
    L"TopologyMicArray2",
    NULL,
    CreateMicArrayMiniportTopology,
    &MicArray1TopoMiniportFilterDescriptor2,
    0, NULL,
    L"WaveMicArray2",
    NULL,
    CreateMiniportWaveRTVirtualAudioDriver,
    &MicArrayWaveMiniportFilterDescriptor,
    0, NULL,
    MICARRAY_DEVICE_MAX_CHANNELS,
    MicArrayPinDeviceFormatsAndModes,
    SIZEOF_ARRAY(MicArrayPinDeviceFormatsAndModes),
    MicArray1TopologyPhysicalConnections,
    SIZEOF_ARRAY(MicArray1TopologyPhysicalConnections),
    ENDPOINT_NO_FLAGS,
    1,      // IpcChannelId = 1
    TRUE,
};

static
ENDPOINT_MINIPAIR MicArray3Miniports =
{
    eMicArrayDevice3,
    L"TopologyMicArray3",
    NULL,
    CreateMicArrayMiniportTopology,
    &MicArray1TopoMiniportFilterDescriptor3,
    0, NULL,
    L"WaveMicArray3",
    NULL,
    CreateMiniportWaveRTVirtualAudioDriver,
    &MicArrayWaveMiniportFilterDescriptor,
    0, NULL,
    MICARRAY_DEVICE_MAX_CHANNELS,
    MicArrayPinDeviceFormatsAndModes,
    SIZEOF_ARRAY(MicArrayPinDeviceFormatsAndModes),
    MicArray1TopologyPhysicalConnections,
    SIZEOF_ARRAY(MicArray1TopologyPhysicalConnections),
    ENDPOINT_NO_FLAGS,
    2,      // IpcChannelId = 2
    TRUE,
};

static
ENDPOINT_MINIPAIR MicArray4Miniports =
{
    eMicArrayDevice4,
    L"TopologyMicArray4",
    NULL,
    CreateMicArrayMiniportTopology,
    &MicArray1TopoMiniportFilterDescriptor4,
    0, NULL,
    L"WaveMicArray4",
    NULL,
    CreateMiniportWaveRTVirtualAudioDriver,
    &MicArrayWaveMiniportFilterDescriptor,
    0, NULL,
    MICARRAY_DEVICE_MAX_CHANNELS,
    MicArrayPinDeviceFormatsAndModes,
    SIZEOF_ARRAY(MicArrayPinDeviceFormatsAndModes),
    MicArray1TopologyPhysicalConnections,
    SIZEOF_ARRAY(MicArray1TopologyPhysicalConnections),
    ENDPOINT_NO_FLAGS,
    3,      // IpcChannelId = 3
    TRUE,
};

//=============================================================================
//
// Render miniport pairs.
//
static
PENDPOINT_MINIPAIR  g_RenderEndpoints[] = 
{
    &SpeakerMiniports,
    &SpeakerMiniports2,
    &SpeakerMiniports3,
    &SpeakerMiniports4,
};

#define g_cRenderEndpoints  (SIZEOF_ARRAY(g_RenderEndpoints))

//=============================================================================
//
// Capture miniport pairs.
//
static
PENDPOINT_MINIPAIR  g_CaptureEndpoints[] =
{
    &MicArray1Miniports,
    &MicArray2Miniports,
    &MicArray3Miniports,
    &MicArray4Miniports,
};

#define g_cCaptureEndpoints (SIZEOF_ARRAY(g_CaptureEndpoints))

//=============================================================================
//
// Total miniports = # endpoints * 2 (topology + wave).
//
#define g_MaxMiniports  ((g_cRenderEndpoints + g_cCaptureEndpoints) * 2)

#endif // _VIRTUALAUDIODRIVER_MINIPAIRS_H_
