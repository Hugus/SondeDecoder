// SondeDecoder.cpp : définit le point d'entrée pour l'application console.
//


#include "stdafx.h"
#include <Mmdeviceapi.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <Audioclient.h>
#include "M10Decoder.h"

//-----------------------------------------------------------
// This function enumerates all active (plugged in) audio
// rendering endpoint devices. It prints the friendly name
// and endpoint ID string of each endpoint device.
//-----------------------------------------------------------
#define EXIT_ON_ERROR(hres)  \
              if (FAILED(hres)) { goto Exit; }
#define SAFE_RELEASE(punk)  \
              if ((punk) != NULL)  \
                { (punk)->Release(); (punk) = NULL; }

const CLSID CLSID_MMDeviceEnumerator = __uuidof( MMDeviceEnumerator );
const IID IID_IMMDeviceEnumerator = __uuidof( IMMDeviceEnumerator );

void PrintEndpointNames ()
{
    HRESULT hr = S_OK;
    IMMDeviceEnumerator *pEnumerator = NULL;
    IMMDeviceCollection *pCollection = NULL;
    IMMDevice *pEndpoint = NULL;
    IPropertyStore *pProps = NULL;
    LPWSTR pwszID = NULL;

    CoInitialize ( NULL );

    hr = CoCreateInstance (
        CLSID_MMDeviceEnumerator, NULL,
        CLSCTX_ALL, IID_IMMDeviceEnumerator,
        (void**)&pEnumerator );
    EXIT_ON_ERROR ( hr )

        hr = pEnumerator->EnumAudioEndpoints (
            eCapture, DEVICE_STATE_ACTIVE,
            &pCollection );
    EXIT_ON_ERROR ( hr )

        UINT  count;
    hr = pCollection->GetCount ( &count );
    EXIT_ON_ERROR ( hr )

        if ( count == 0 )
        {
            printf ( "No endpoints found.\n" );
        }

    // Each loop prints the name of an endpoint device.
    for ( ULONG i = 0; i < count; i++ )
    {
        // Get pointer to endpoint number i.
        hr = pCollection->Item ( i, &pEndpoint );
        EXIT_ON_ERROR ( hr )

            // Get the endpoint ID string.
            hr = pEndpoint->GetId ( &pwszID );
        EXIT_ON_ERROR ( hr )

            hr = pEndpoint->OpenPropertyStore (
                STGM_READ, &pProps );
        EXIT_ON_ERROR ( hr )

            PROPVARIANT varName;
        // Initialize container for property value.
        PropVariantInit ( &varName );

        // Get the endpoint's friendly-name property.
        hr = pProps->GetValue (
            PKEY_Device_FriendlyName, &varName );
        EXIT_ON_ERROR ( hr )

            // Print endpoint friendly name and endpoint ID.
            printf ( "Endpoint %d: \"%S\" (%S)\n",
                i, varName.pwszVal, pwszID );

        CoTaskMemFree ( pwszID );
        pwszID = NULL;
        PropVariantClear ( &varName );
        SAFE_RELEASE ( pProps )
            SAFE_RELEASE ( pEndpoint )
    }
    SAFE_RELEASE ( pEnumerator )
        SAFE_RELEASE ( pCollection )
        return ;

Exit:
    printf ( "Error!\n" );
    CoTaskMemFree ( pwszID );
    SAFE_RELEASE ( pEnumerator )
        SAFE_RELEASE ( pCollection )
        SAFE_RELEASE ( pEndpoint )
        SAFE_RELEASE ( pProps )
}

//-----------------------------------------------------------
// Record an audio stream from the default audio capture
// device. The RecordAudioStream function allocates a shared
// buffer big enough to hold one second of PCM audio data.
// The function uses this buffer to stream data from the
// capture device. The main loop runs every 1/2 second.
//-----------------------------------------------------------

// REFERENCE_TIME time units per second and per millisecond
#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000

const IID IID_IAudioClient = __uuidof( IAudioClient );
const IID IID_IAudioCaptureClient = __uuidof( IAudioCaptureClient );


HRESULT RecordAudioStream ( M10Decoder *pMySink )
{
    HRESULT hr;
    REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC;
    REFERENCE_TIME hnsActualDuration;
    UINT32 bufferFrameCount;
    UINT32 numFramesAvailable;
    IMMDeviceEnumerator *pEnumerator = NULL;
    IMMDevice *pDevice = NULL;
    IAudioClient *pAudioClient = NULL;
    IAudioCaptureClient *pCaptureClient = NULL;
    WAVEFORMATEX *pwfx = NULL;
    UINT32 packetLength = 0;
    BOOL bDone = FALSE;
    uint8_t *pData;
    DWORD flags;

    hr = CoCreateInstance (
        CLSID_MMDeviceEnumerator, NULL,
        CLSCTX_ALL, IID_IMMDeviceEnumerator,
        (void**)&pEnumerator );
    EXIT_ON_ERROR ( hr )

        hr = pEnumerator->GetDefaultAudioEndpoint (
            eCapture, eConsole, &pDevice );
    EXIT_ON_ERROR ( hr )

        hr = pDevice->Activate (
            IID_IAudioClient, CLSCTX_ALL,
            NULL, (void**)&pAudioClient );
    EXIT_ON_ERROR ( hr )

        hr = pAudioClient->GetMixFormat ( &pwfx );
    EXIT_ON_ERROR ( hr )

        hr = pAudioClient->Initialize (
            AUDCLNT_SHAREMODE_SHARED,
            0,
            hnsRequestedDuration,
            0,
            pwfx,
            NULL );
    EXIT_ON_ERROR ( hr )

        // Get the size of the allocated buffer.
        hr = pAudioClient->GetBufferSize ( &bufferFrameCount );
    EXIT_ON_ERROR ( hr )

        hr = pAudioClient->GetService (
            IID_IAudioCaptureClient,
            (void**)&pCaptureClient );
    EXIT_ON_ERROR ( hr )

        // Notify the audio sink which format to use.
        hr = pMySink->SetFormat ( pwfx );
    EXIT_ON_ERROR ( hr )

        // Calculate the actual duration of the allocated buffer.
        hnsActualDuration = (double)REFTIMES_PER_SEC *
        bufferFrameCount / pwfx->nSamplesPerSec;
        
        printf( "Buffer duration : %f s\n", (double)bufferFrameCount / pwfx->nSamplesPerSec ) ;

    hr = pAudioClient->Start ();  // Start recording.
    EXIT_ON_ERROR ( hr )

        // Each loop fills about half of the shared buffer.
        while ( bDone == FALSE )
        {
            // Sleep for half the buffer duration.
            Sleep ( (double)hnsActualDuration / REFTIMES_PER_MILLISEC / 2 );

            hr = pCaptureClient->GetNextPacketSize ( &packetLength );
            EXIT_ON_ERROR ( hr )

                while ( packetLength != 0 )
                {
                    // Get the available data in the shared buffer.
                    hr = pCaptureClient->GetBuffer (
                        &pData,
                        &numFramesAvailable,
                        &flags, NULL, NULL );
                    EXIT_ON_ERROR ( hr )

                        if ( flags & AUDCLNT_BUFFERFLAGS_SILENT )
                        {
                            pData = NULL;  // Tell CopyData to write silence.
                        }
                        else if ( flags & AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY )
                        {
                            printf ( "Data discontinuity error.\n" ) ;
                        }
                        else if ( flags & AUDCLNT_BUFFERFLAGS_TIMESTAMP_ERROR )
                        {
                            printf ( "Timestamp error.\n" ) ;
                        }

                    // Copy the available capture data to the audio sink.
                    hr = pMySink->CopyData (
                        pData, numFramesAvailable, &bDone );
                    EXIT_ON_ERROR ( hr )

                        hr = pCaptureClient->ReleaseBuffer ( numFramesAvailable );
                    EXIT_ON_ERROR ( hr )

                        hr = pCaptureClient->GetNextPacketSize ( &packetLength );
                    EXIT_ON_ERROR ( hr )
                }
        }

    hr = pAudioClient->Stop ();  // Stop recording.
    EXIT_ON_ERROR ( hr )

        Exit:
    CoTaskMemFree ( pwfx );
    SAFE_RELEASE ( pEnumerator )
        SAFE_RELEASE ( pDevice )
        SAFE_RELEASE ( pAudioClient )
        SAFE_RELEASE ( pCaptureClient )

        return hr;
}

int main ()
{
    PrintEndpointNames () ;
    M10Decoder m10Decoder ;
    RecordAudioStream ( &m10Decoder ) ;
    return 0;
}


