#include "stdafx.h"
#include "M10Decoder.h"
#include <iostream>

M10Decoder::M10Decoder ()
{
}


M10Decoder::~M10Decoder ()
{
}

HRESULT M10Decoder::SetFormat ( WAVEFORMATEX * pwfx )
{
    m_bitsPerSample = pwfx->wBitsPerSample ;
    m_samplesPerSec = pwfx->nSamplesPerSec ;
    return NOERROR;
}

HRESULT M10Decoder::CopyData ( BYTE * pData, UINT32 numFramesAvailable, BOOL * bDone )
{
    *bDone = true ;
    for ( UINT32 i = 0 ; i < numFramesAvailable ; ++i )
    {
        std::cout << (int)pData[i] << std::endl ;
    }
    return NOERROR;
}
