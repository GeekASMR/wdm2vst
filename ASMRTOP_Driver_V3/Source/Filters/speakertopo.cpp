/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    speakertopo.cpp

Abstract:

    Implementation of topology miniport for the speaker (internal).
--*/

#pragma warning (disable : 4127)

#include "definitions.h"
#include "endpoints.h"
#include "mintopo.h"
#include "speakertopo.h"
#include "speakertoptable.h"


#pragma code_seg("PAGE")
//=============================================================================
NTSTATUS
PropertyHandler_SpeakerTopoFilter
( 
    _In_ PPCPROPERTY_REQUEST      PropertyRequest 
)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(PropertyRequest);

    // No jack properties for virtual audio device
    return STATUS_INVALID_DEVICE_REQUEST;
} // PropertyHandler_SpeakerTopoFilter

//=============================================================================
NTSTATUS
PropertyHandler_SpeakerTopology
(
    _In_ PPCPROPERTY_REQUEST      PropertyRequest
)
/*++

Routine Description:

  Redirects property request to miniport object

Arguments:

  PropertyRequest -

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    ASSERT(PropertyRequest);

    DPF_ENTER(("[PropertyHandler_SpeakerTopology]"));

    // PropertryRequest structure is filled by portcls. 
    // MajorTarget is a pointer to miniport object for miniports.
    //
    PCMiniportTopology pMiniport = (PCMiniportTopology)PropertyRequest->MajorTarget;

    return pMiniport->PropertyHandlerGeneric(PropertyRequest);
} // PropertyHandler_SpeakerTopology

#pragma code_seg()
