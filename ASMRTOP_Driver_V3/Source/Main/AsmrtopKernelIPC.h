#ifndef _ASMRTOP_KERNEL_IPC_H_
#define _ASMRTOP_KERNEL_IPC_H_

#include <ntddk.h>
#include "branding.h"

// Must match the layout in AsmrtopIPC.h exactly
typedef struct _IpcAudioBuffer
{
    volatile ULONG writePos;
    volatile ULONG readPos;
    float ringL[131072];
    float ringR[131072];
} IpcAudioBuffer, *PIpcAudioBuffer;

extern "C" {
    NTKERNELAPI
    NTSTATUS
    MmMapViewInSystemSpace (
        _In_ PVOID Section,
        _Outptr_result_bytebuffer_(*ViewSize) PVOID *MappedBase,
        _Inout_ PSIZE_T ViewSize
        );

    NTKERNELAPI
    NTSTATUS
    MmUnmapViewInSystemSpace (
        _In_ PVOID MappedBase
        );
}

class CSharedMemoryBridgeKernel
{
public:
    CSharedMemoryBridgeKernel() : m_SectionHandle(NULL), m_SectionObject(NULL), m_SharedMemory(NULL), m_Mdl(NULL) {}

    ~CSharedMemoryBridgeKernel()
    {
        Cleanup();
    }

    NTSTATUS Initialize(PCWSTR directionPrefix, ULONG channelId)
    {
        NTSTATUS status;
        UNICODE_STRING sectionName;
        OBJECT_ATTRIBUTES objectAttributes;
        LARGE_INTEGER sectionSize;

        // E.g. L"\\BaseNamedObjects\\AsmrtopWDM_PLAY_0"
        WCHAR nameBuffer[256];
        RtlStringCchPrintfW(nameBuffer, 256, L"\\BaseNamedObjects\\" IPC_BASE_NAME L"_%s_%lu", directionPrefix, channelId);
        RtlInitUnicodeString(&sectionName, nameBuffer);

        UCHAR sdBuf[64];
        PSECURITY_DESCRIPTOR pSd = (PSECURITY_DESCRIPTOR)sdBuf;
        RtlCreateSecurityDescriptor(pSd, SECURITY_DESCRIPTOR_REVISION);
        RtlSetDaclSecurityDescriptor(pSd, TRUE, NULL, FALSE);

        InitializeObjectAttributes(&objectAttributes,
                                 &sectionName,
                                 OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                 NULL,
                                 pSd);

        sectionSize.QuadPart = sizeof(IpcAudioBuffer);

        status = ZwCreateSection(&m_SectionHandle,
                                 SECTION_ALL_ACCESS,
                                 &objectAttributes,
                                 &sectionSize,
                                 PAGE_READWRITE,
                                 SEC_COMMIT,
                                 NULL);

        if (status == STATUS_OBJECT_NAME_COLLISION || status == 0xC0000035L)
        {
            status = ZwOpenSection(&m_SectionHandle, SECTION_ALL_ACCESS, &objectAttributes);
        }

        if (NT_SUCCESS(status))
        {
            status = ObReferenceObjectByHandle(m_SectionHandle,
                                               SECTION_MAP_READ | SECTION_MAP_WRITE,
                                               NULL,
                                               KernelMode,
                                               &m_SectionObject,
                                               NULL);
            if (NT_SUCCESS(status))
            {
                SIZE_T viewSize = sizeof(IpcAudioBuffer);
                status = MmMapViewInSystemSpace(m_SectionObject, (PVOID*)&m_SharedMemory, &viewSize);

                if (NT_SUCCESS(status) && m_SharedMemory != NULL)
                {
                    m_Mdl = IoAllocateMdl(m_SharedMemory, sizeof(IpcAudioBuffer), FALSE, FALSE, NULL);
                    if (m_Mdl)
                    {
                        __try
                        {
                            MmProbeAndLockPages(m_Mdl, KernelMode, IoModifyAccess);
                            // Zero initialize once locked
                            RtlZeroMemory(m_SharedMemory, sizeof(IpcAudioBuffer));
                        }
                        __except(EXCEPTION_EXECUTE_HANDLER)
                        {
                            IoFreeMdl(m_Mdl);
                            m_Mdl = NULL;
                            MmUnmapViewInSystemSpace(m_SharedMemory);
                            m_SharedMemory = NULL;
                            status = STATUS_UNSUCCESSFUL;
                        }
                    }
                    else
                    {
                        MmUnmapViewInSystemSpace(m_SharedMemory);
                        m_SharedMemory = NULL;
                        status = STATUS_INSUFFICIENT_RESOURCES;
                    }
                }

                if (!NT_SUCCESS(status))
                {
                    ObDereferenceObject(m_SectionObject);
                    m_SectionObject = NULL;
                }
            }
        }

        if (!NT_SUCCESS(status))
        {
            Cleanup();
        }

        return status;
    }

    void Cleanup()
    {
        if (m_Mdl)
        {
            MmUnlockPages(m_Mdl);
            IoFreeMdl(m_Mdl);
            m_Mdl = NULL;
        }
        if (m_SharedMemory)
        {
            MmUnmapViewInSystemSpace(m_SharedMemory);
            m_SharedMemory = NULL;
        }
        if (m_SectionObject)
        {
            ObDereferenceObject(m_SectionObject);
            m_SectionObject = NULL;
        }
        if (m_SectionHandle)
        {
            ZwClose(m_SectionHandle);
            m_SectionHandle = NULL;
        }
    }

    // Capture system playback and write to IPC ring buffer (WDM -> VST)
    void WritePcmToRing(const BYTE* pcmBuffer, ULONG byteCount, ULONG numChannels, ULONG bitDepth, bool isFloat)
    {
        if (!m_SharedMemory) return;

        ULONG bytesPerSample = bitDepth / 8;
        ULONG frameSize = bytesPerSample * numChannels;
        ULONG numFrames = byteCount / frameSize;

        ULONG currentWritePos = m_SharedMemory->writePos;

        for (ULONG i = 0; i < numFrames; ++i)
        {
            float lVal = 0.0f;
            float rVal = 0.0f;

            if (bitDepth == 16)
            {
                SHORT* pSample = (SHORT*)(pcmBuffer + i * frameSize);
                lVal = (float)pSample[0] / 32768.0f;
                rVal = numChannels > 1 ? (float)pSample[1] / 32768.0f : lVal;
            }
            else if (bitDepth == 24)
            {
                // 24-bit PCM conversion
                BYTE* pBase = (BYTE*)(pcmBuffer + i * frameSize);
                LONG l24 = (pBase[0]) | (pBase[1] << 8) | (pBase[2] << 16);
                if (l24 & 0x00800000) l24 |= 0xFF000000;
                lVal = (float)l24 / 8388608.0f;

                if (numChannels > 1)
                {
                    LONG r24 = (pBase[3]) | (pBase[4] << 8) | (pBase[5] << 16);
                    if (r24 & 0x00800000) r24 |= 0xFF000000;
                    rVal = (float)r24 / 8388608.0f;
                }
                else
                {
                    rVal = lVal;
                }
            }
            else if (bitDepth == 32)
            {
                if (isFloat)
                {
                    float* pSample = (float*)(pcmBuffer + i * frameSize);
                    lVal = pSample[0];
                    rVal = numChannels > 1 ? pSample[1] : lVal;
                }
                else
                {
                    LONG* pSample = (LONG*)(pcmBuffer + i * frameSize);
                    lVal = (float)pSample[0] / 2147483648.0f;
                    rVal = numChannels > 1 ? (float)pSample[1] / 2147483648.0f : lVal;
                }
            }

            ULONG idx = (currentWritePos + i) & 131071;
            m_SharedMemory->ringL[idx] = lVal;
            m_SharedMemory->ringR[idx] = rVal;
        }

        m_SharedMemory->writePos = currentWritePos + numFrames;
    }

    // Read from IPC ring buffer and inject into system capture (VST -> WDM)
    void ReadRingToPcm(BYTE* pcmBuffer, ULONG byteCount, ULONG numChannels, ULONG bitDepth, bool isFloat)
    {
        if (!m_SharedMemory)
        {
            RtlZeroMemory(pcmBuffer, byteCount);
            return;
        }

        ULONG bytesPerSample = bitDepth / 8;
        ULONG frameSize = bytesPerSample * numChannels;
        ULONG numFrames = byteCount / frameSize;

        ULONG w = m_SharedMemory->writePos;
        ULONG r = m_SharedMemory->readPos;
        LONG available = (LONG)(w - r);

        if (available < 0 || available > 131072) {
            r = w;
            available = 0;
        }

        for (ULONG i = 0; i < numFrames; ++i)
        {
            float lVal = 0.0f;
            float rVal = 0.0f;

            if (available > 0)
            {
                ULONG idx = r & 131071;
                lVal = m_SharedMemory->ringL[idx];
                rVal = m_SharedMemory->ringR[idx];
                r++;
                available--;
            }

            if (lVal > 1.0f) lVal = 1.0f; else if (lVal < -1.0f) lVal = -1.0f;
            if (rVal > 1.0f) rVal = 1.0f; else if (rVal < -1.0f) rVal = -1.0f;

            if (bitDepth == 16)
            {
                SHORT* pSample = (SHORT*)(pcmBuffer + i * frameSize);
                pSample[0] = (SHORT)(lVal * 32767.0f);
                if (numChannels > 1) pSample[1] = (SHORT)(rVal * 32767.0f);
            }
            else if (bitDepth == 24)
            {
                BYTE* pBase = (BYTE*)(pcmBuffer + i * frameSize);
                LONG l24 = (LONG)(lVal * 8388607.0f);
                pBase[0] = (BYTE)(l24 & 0xFF);
                pBase[1] = (BYTE)((l24 >> 8) & 0xFF);
                pBase[2] = (BYTE)((l24 >> 16) & 0xFF);

                if (numChannels > 1)
                {
                    LONG r24 = (LONG)(rVal * 8388607.0f);
                    pBase[3] = (BYTE)(r24 & 0xFF);
                    pBase[4] = (BYTE)((r24 >> 8) & 0xFF);
                    pBase[5] = (BYTE)((r24 >> 16) & 0xFF);
                }
            }
            else if (bitDepth == 32)
            {
                if (isFloat)
                {
                    float* pSample = (float*)(pcmBuffer + i * frameSize);
                    pSample[0] = lVal;
                    if (numChannels > 1) pSample[1] = rVal;
                }
                else
                {
                    LONG* pSample = (LONG*)(pcmBuffer + i * frameSize);
                    pSample[0] = (LONG)(lVal * 2147483647.0f);
                    if (numChannels > 1) pSample[1] = (LONG)(rVal * 2147483647.0f);
                }
            }
        }
        m_SharedMemory->readPos = r;
    }

private:
    HANDLE m_SectionHandle;
    PVOID m_SectionObject;
    PIpcAudioBuffer m_SharedMemory;
    PMDL m_Mdl;
};

#endif
