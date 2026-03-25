/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    micarray1toptable.h

Abstract:

    Declaration of topology tables for the mic array.
    Each of the 4 mic endpoints gets a unique Name GUID
    so Windows shows distinct endpoint names.

--*/

#ifndef _VIRTUALAUDIODRIVER_MICARRAY1TOPTABLE_H_
#define _VIRTUALAUDIODRIVER_MICARRAY1TOPTABLE_H_

#include "branding.h"

// Unique Name GUIDs for each mic endpoint pin.
// These are referenced by KsName entries in the INF.
// {E6E750E0-0002-4000-A000-000000000001} .. 04
DEFINE_GUID(KSNAME_Mic1, 0xE6E750E0, 0x0002, 0x4000, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01);
DEFINE_GUID(KSNAME_Mic2, 0xE6E750E0, 0x0002, 0x4000, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02);
DEFINE_GUID(KSNAME_Mic3, 0xE6E750E0, 0x0002, 0x4000, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03);
DEFINE_GUID(KSNAME_Mic4, 0xE6E750E0, 0x0002, 0x4000, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04);

//=============================================================================
static
KSDATARANGE MicArray1TopoPinDataRangesBridge[] =
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
PKSDATARANGE MicArray1TopoPinDataRangePointersBridge[] =
{
  &MicArray1TopoPinDataRangesBridge[0]
};

//=============================================================================
// Macro: define mic topology pins with a unique Name GUID on the input pin
#define DEFINE_MIC_TOPO_PINS(varName, nameGuid) \
static PCPIN_DESCRIPTOR varName[] = \
{ \
  /* KSPIN_TOPO_MIC_ELEMENTS */ \
  { \
    0, 0, 0, NULL, \
    { 0, NULL, 0, NULL, \
      SIZEOF_ARRAY(MicArray1TopoPinDataRangePointersBridge), \
      MicArray1TopoPinDataRangePointersBridge, \
      KSPIN_DATAFLOW_IN, KSPIN_COMMUNICATION_NONE, \
      &KSNODETYPE_LINE_CONNECTOR, &nameGuid, 0 \
    } \
  }, \
  /* KSPIN_TOPO_BRIDGE */ \
  { \
    0, 0, 0, NULL, \
    { 0, NULL, 0, NULL, \
      SIZEOF_ARRAY(MicArray1TopoPinDataRangePointersBridge), \
      MicArray1TopoPinDataRangePointersBridge, \
      KSPIN_DATAFLOW_OUT, KSPIN_COMMUNICATION_NONE, \
      &KSCATEGORY_AUDIO, NULL, 0 \
    } \
  } \
};

// Create 4 pin descriptor arrays, one per mic endpoint
DEFINE_MIC_TOPO_PINS(MicArray1TopoMiniportPins1, KSNAME_Mic1)
DEFINE_MIC_TOPO_PINS(MicArray1TopoMiniportPins2, KSNAME_Mic2)
DEFINE_MIC_TOPO_PINS(MicArray1TopoMiniportPins3, KSNAME_Mic3)
DEFINE_MIC_TOPO_PINS(MicArray1TopoMiniportPins4, KSNAME_Mic4)

// Backward compat alias
#define MicArray1TopoMiniportPins MicArray1TopoMiniportPins1

//=============================================================================
static
PCPROPERTY_ITEM MicArray1PropertiesVolume[] =
{
    {
    &KSPROPSETID_Audio,
    KSPROPERTY_AUDIO_VOLUMELEVEL,
    KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
    PropertyHandler_MicArrayTopology
    }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationMicArray1Volume, MicArray1PropertiesVolume);

//=============================================================================
static
PCPROPERTY_ITEM MicArray1PropertiesMute[] =
{
  {
    &KSPROPSETID_Audio,
    KSPROPERTY_AUDIO_MUTE,
    KSPROPERTY_TYPE_GET | KSPROPERTY_TYPE_SET | KSPROPERTY_TYPE_BASICSUPPORT,
    PropertyHandler_MicArrayTopology
  }
};

DEFINE_PCAUTOMATION_TABLE_PROP(AutomationMicArray1Mute, MicArray1PropertiesMute);

//=============================================================================
static
PCNODE_DESCRIPTOR MicArray1TopologyNodes[] =
{
    // KSNODE_TOPO_VOLUME
    {
      0,                              // Flags
      &AutomationMicArray1Volume,     // AutomationTable
      &KSNODETYPE_VOLUME,             // Type
      &KSAUDFNAME_MIC_VOLUME          // Name
    },
    // KSNODE_TOPO_MUTE
    {
      0,                              // Flags
      &AutomationMicArray1Mute,       // AutomationTable
      &KSNODETYPE_MUTE,               // Type
      &KSAUDFNAME_MIC_MUTE            // Name
    }
};

C_ASSERT(KSNODE_TOPO_VOLUME == 0);
C_ASSERT(KSNODE_TOPO_MUTE == 1);

static
PCCONNECTION_DESCRIPTOR MicArray1TopoMiniportConnections[] =
{
    //  FromNode,                 FromPin,                    ToNode,                 ToPin
    {   PCFILTER_NODE,            KSPIN_TOPO_MIC_ELEMENTS,    KSNODE_TOPO_VOLUME,     1 },
    {   KSNODE_TOPO_VOLUME,       0,                          KSNODE_TOPO_MUTE,       1 },
    {   KSNODE_TOPO_MUTE,         0,                          PCFILTER_NODE,          KSPIN_TOPO_BRIDGE }
};

//=============================================================================
static
PCAUTOMATION_TABLE AutomationMicArray1TopoFilter =
{
    0, 0, NULL,   // Properties
    0, 0, NULL,   // Methods
    0, 0, NULL    // Events
};

//=============================================================================
// Macro: define per-endpoint filter descriptor
#define DEFINE_MIC_FILTER_DESC(varName, pinsVar) \
static PCFILTER_DESCRIPTOR varName = \
{ \
  0, \
  &AutomationMicArray1TopoFilter, \
  sizeof(PCPIN_DESCRIPTOR), \
  SIZEOF_ARRAY(pinsVar), \
  pinsVar, \
  sizeof(PCNODE_DESCRIPTOR), \
  SIZEOF_ARRAY(MicArray1TopologyNodes), \
  MicArray1TopologyNodes, \
  SIZEOF_ARRAY(MicArray1TopoMiniportConnections), \
  MicArray1TopoMiniportConnections, \
  0, \
  NULL \
};

// Create 4 filter descriptors, one per mic endpoint
DEFINE_MIC_FILTER_DESC(MicArray1TopoMiniportFilterDescriptor,  MicArray1TopoMiniportPins1)
DEFINE_MIC_FILTER_DESC(MicArray1TopoMiniportFilterDescriptor2, MicArray1TopoMiniportPins2)
DEFINE_MIC_FILTER_DESC(MicArray1TopoMiniportFilterDescriptor3, MicArray1TopoMiniportPins3)
DEFINE_MIC_FILTER_DESC(MicArray1TopoMiniportFilterDescriptor4, MicArray1TopoMiniportPins4)

#endif // _VIRTUALAUDIODRIVER_MICARRAY1TOPTABLE_H_
