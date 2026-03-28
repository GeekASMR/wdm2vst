/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    speakertoptable.h

Abstract:

    Declaration of topology tables for speaker endpoints.
    Each of the 4 speaker endpoints gets a unique Name GUID
    so Windows shows distinct endpoint names.
--*/

#ifndef _VIRTUALAUDIODRIVER_SPEAKERTOPTABLE_H_
#define _VIRTUALAUDIODRIVER_SPEAKERTOPTABLE_H_

#include "branding.h"

// Unique Name GUIDs for each speaker endpoint pin.
// These are referenced by KsName entries in the INF to provide display names.
// {E6E750E0-0001-4000-A000-000000000001} .. 04
DEFINE_GUID(KSNAME_Speaker1, 0xE6E750E0, 0x0001, 0x4000, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01);
DEFINE_GUID(KSNAME_Speaker2, 0xE6E750E0, 0x0001, 0x4000, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02);
DEFINE_GUID(KSNAME_Speaker3, 0xE6E750E0, 0x0001, 0x4000, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03);
DEFINE_GUID(KSNAME_Speaker4, 0xE6E750E0, 0x0001, 0x4000, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04);

//=============================================================================
static
KSDATARANGE SpeakerTopoPinDataRangesBridge[] =
{
 {
   sizeof(KSDATARANGE),
   0,
   0,
   0,
   STATICGUIDOF(KSDATAFORMAT_TYPE_AUDIO),
   STATICGUIDOF(KSDATAFORMAT_SUBTYPE_ANALOG),
   STATICGUIDOF(KSDATAFORMAT_SPECIFIER_NONE)
 }
};

//=============================================================================
static
PKSDATARANGE SpeakerTopoPinDataRangePointersBridge[] =
{
  &SpeakerTopoPinDataRangesBridge[0]
};

//=============================================================================
// Macro: define a pair of topology pins (source + output) with a unique Name GUID
#define DEFINE_SPEAKER_TOPO_PINS(varName, nameGuid) \
static PCPIN_DESCRIPTOR varName[] = \
{ \
  /* KSPIN_TOPO_WAVEOUT_SOURCE */ \
  { \
    0, 0, 0, NULL, \
    { 0, NULL, 0, NULL, \
      SIZEOF_ARRAY(SpeakerTopoPinDataRangePointersBridge), \
      SpeakerTopoPinDataRangePointersBridge, \
      KSPIN_DATAFLOW_IN, KSPIN_COMMUNICATION_NONE, \
      &KSCATEGORY_AUDIO, NULL, 0 \
    } \
  }, \
  /* KSPIN_TOPO_LINEOUT_DEST */ \
  { \
    0, 0, 0, NULL, \
    { 0, NULL, 0, NULL, \
      SIZEOF_ARRAY(SpeakerTopoPinDataRangePointersBridge), \
      SpeakerTopoPinDataRangePointersBridge, \
      KSPIN_DATAFLOW_OUT, KSPIN_COMMUNICATION_NONE, \
      &KSNODETYPE_LINE_CONNECTOR, NULL, 0 \
    } \
  } \
};

// Create 4 pin descriptor arrays, one per speaker endpoint
DEFINE_SPEAKER_TOPO_PINS(SpeakerTopoMiniportPins1, KSNAME_Speaker1)
DEFINE_SPEAKER_TOPO_PINS(SpeakerTopoMiniportPins2, KSNAME_Speaker2)
DEFINE_SPEAKER_TOPO_PINS(SpeakerTopoMiniportPins3, KSNAME_Speaker3)
DEFINE_SPEAKER_TOPO_PINS(SpeakerTopoMiniportPins4, KSNAME_Speaker4)

// Backward compat alias
#define SpeakerTopoMiniportPins SpeakerTopoMiniportPins1

static
PCNODE_DESCRIPTOR SpeakerTopologyNodes[] =
{
    // KSNODE_TOPO_MUTE
    {
        0,                          // Flags
        NULL,                       // AutomationTable
        &KSNODETYPE_MUTE,           // Type
        &KSAUDFNAME_WAVE_MUTE       // Name
    }
};

// Physical connections
static
PHYSICALCONNECTIONTABLE SpeakerTopologyPhysicalConnections[] =
{
    {
        KSPIN_TOPO_WAVEOUT_SOURCE,  // TopologyIn
        KSPIN_WAVE_RENDER3_SOURCE,   // WaveOut
        CONNECTIONTYPE_WAVE_OUTPUT
    }
};

//=============================================================================
static
PCCONNECTION_DESCRIPTOR SpeakerTopoMiniportConnections[] =
{
    //  FromNode,                 FromPin,                    ToNode,                 ToPin
    {   PCFILTER_NODE,            KSPIN_TOPO_WAVEOUT_SOURCE,  KSNODE_TOPO_MUTE,       1 },
    {   KSNODE_TOPO_MUTE,         0,                          PCFILTER_NODE,          KSPIN_TOPO_LINEOUT_DEST }
};

//=============================================================================
static
PCAUTOMATION_TABLE AutomationSpeakerTopoFilter =
{
    0, 0, NULL,   // Properties
    0, 0, NULL,   // Methods
    0, 0, NULL    // Events
};

//=============================================================================
// Macro: define a filter descriptor using a specific pin array
#define DEFINE_SPEAKER_FILTER_DESC(varName, pinsVar) \
static PCFILTER_DESCRIPTOR varName = \
{ \
  0, \
  &AutomationSpeakerTopoFilter, \
  sizeof(PCPIN_DESCRIPTOR), \
  SIZEOF_ARRAY(pinsVar), \
  pinsVar, \
  sizeof(PCNODE_DESCRIPTOR), \
  SIZEOF_ARRAY(SpeakerTopologyNodes), \
  SpeakerTopologyNodes, \
  SIZEOF_ARRAY(SpeakerTopoMiniportConnections), \
  SpeakerTopoMiniportConnections, \
  0, \
  NULL \
};

// Create 4 filter descriptors, one per speaker endpoint
DEFINE_SPEAKER_FILTER_DESC(SpeakerTopoMiniportFilterDescriptor,  SpeakerTopoMiniportPins1)
DEFINE_SPEAKER_FILTER_DESC(SpeakerTopoMiniportFilterDescriptor2, SpeakerTopoMiniportPins2)
DEFINE_SPEAKER_FILTER_DESC(SpeakerTopoMiniportFilterDescriptor3, SpeakerTopoMiniportPins3)
DEFINE_SPEAKER_FILTER_DESC(SpeakerTopoMiniportFilterDescriptor4, SpeakerTopoMiniportPins4)

#endif // _VIRTUALAUDIODRIVER_SPEAKERTOPTABLE_H_
