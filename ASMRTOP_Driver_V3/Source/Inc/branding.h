/*++
Module Name:
    branding.h
Abstract:
    Brand configuration for ASMRTOP / Public builds.
    Comment out ASMRTOP_BRAND to build the public version.
--*/

#ifndef _ASMRTOP_BRANDING_H_
#define _ASMRTOP_BRANDING_H_

// ============================================================================
// Uncomment for ASMRTOP branded build, comment out for public build
// ============================================================================
//#define ASMRTOP_BRAND

#ifdef ASMRTOP_BRAND

#define DRIVER_DEVICE_DESC      L"ASMRTOP Audio Router"
#define DRIVER_SVC_DESC         L"ASMRTOP Audio Router"

#define SPEAKER_NAME_1          L"ASMR Audio 1/2"
#define SPEAKER_NAME_2          L"ASMR Audio 3/4"
#define SPEAKER_NAME_3          L"ASMR Audio 5/6"
#define SPEAKER_NAME_4          L"ASMR Audio 7/8"

#define MIC_NAME_1              L"ASMR Mic 1/2"
#define MIC_NAME_2              L"ASMR Mic 3/4"
#define MIC_NAME_3              L"ASMR Mic 5/6"
#define MIC_NAME_4              L"ASMR Mic 7/8"

#define IPC_BASE_NAME           L"AsmrtopWDM"

#else  // Public version

#define DRIVER_DEVICE_DESC      L"Virtual Audio Router"
#define DRIVER_SVC_DESC         L"Virtual Audio Router"

#define SPEAKER_NAME_1          L"Virtual 1/2"
#define SPEAKER_NAME_2          L"Virtual 3/4"
#define SPEAKER_NAME_3          L"Virtual 5/6"
#define SPEAKER_NAME_4          L"Virtual 7/8"

#define MIC_NAME_1              L"Mic 1/2"
#define MIC_NAME_2              L"Mic 3/4"
#define MIC_NAME_3              L"Mic 5/6"
#define MIC_NAME_4              L"Mic 7/8"

#define IPC_BASE_NAME           L"VirtualAudioWDM"

#endif // ASMRTOP_BRAND

// Number of render/capture endpoint pairs
#define NUM_ENDPOINT_PAIRS      4

#endif // _ASMRTOP_BRANDING_H_
