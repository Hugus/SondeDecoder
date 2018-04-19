#pragma once

#include <Mmdeviceapi.h>

class M10Decoder
{
public:
    M10Decoder ();
    ~M10Decoder ();

    HRESULT SetFormat ( WAVEFORMATEX *pwfx );
    HRESULT CopyData ( BYTE * pData, UINT32 numFramesAvailable, BOOL * bDone );

};

