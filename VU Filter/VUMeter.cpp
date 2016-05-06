//------------------------------------------------------------------------------
// File: VUMeter.cpp
//
// Desc: Implementation file for the VUMeter filter
//
// All rights reserved.
//------------------------------------------------------------------------------

#include <streams.h>     // Active Movie (includes windows.h)
#include <initguid.h>    // declares DEFINE_GUID to declare an EXTERN_C const.
#include <math.h>

#include "qedit.h"
#include "VUMeter.h"

#pragma warning(disable: 4244)	// disable loss of data for conversion double to long

// Local helper functions
IPin * GetInPin( IBaseFilter * pFilter, int PinNum );
IPin * GetOutPin( IBaseFilter * pFilter, int PinNum );

// Setup data - allows the self-registration to work.
const AMOVIESETUP_MEDIATYPE sudIpPinTypes =
{
	&MEDIATYPE_NULL,		// MajorType
	&MEDIASUBTYPE_NULL		// MinorType
};

const AMOVIESETUP_PIN sudIpPin =
{
    L"Input",                     // The Pins name
    FALSE,                        // Is rendered
    FALSE,                        // Is an output pin
    FALSE,                        // Allowed none
    FALSE,                        // Allowed many
    &CLSID_NULL,                  // Connects to filter
    NULL,                         // Connects to pin
    1,                            // Number of types
    &sudIpPinTypes                // Pin details
};

const AMOVIESETUP_FILTER sudVUMeter =
{
	&CLSID_VUMeter,				// Filter clsID
	L"VU Meter",				// String name
	MERIT_DO_NOT_USE,			// Filter merit
	1,							// Number of pins
	&sudIpPin					// Pin details
};

// Needed for the CreateInstance mechanism
CFactoryTemplate g_Templates[]=
{
    { L"VU Meter"
	, &CLSID_VUMeter
	, CVUMeterFilter::CreateInstance
	, NULL
	, &sudVUMeter }

};

int g_cTemplates = sizeof(g_Templates)/sizeof(g_Templates[0]);


/******************************Public*Routine******************************\
*
* Exported entry points for registration and unregistration (in this case 
* they only call through to default implmentations).
*
***************************************************************************/
STDAPI DllRegisterServer()
{
    return AMovieDllRegisterServer2(TRUE);
}

STDAPI DllUnregisterServer()
{
    return AMovieDllRegisterServer2(FALSE);
}

//
// CreateInstance
//
// Provide the way for COM to create a CVUMeterSample object
//
CUnknown * WINAPI CVUMeterFilter::CreateInstance(LPUNKNOWN punk, HRESULT *phr) 
{
    // assuming we don't want to modify the data
    CVUMeterFilter *pNewObject = new CVUMeterFilter(punk, phr);

    if (pNewObject == NULL)
	{
        *phr = E_OUTOFMEMORY;
    }

    return pNewObject;   
} // CreateInstance


//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------

CVUMeterFilter::CVUMeterFilter(LPUNKNOWN pUnk, HRESULT * phr) :
	CBaseRenderer(CLSID_VUMeter, NAME("VU Meter Filter"), pUnk, phr)
{
	ZeroMemory(&m_wfx, sizeof(WAVEFORMATEX));
} // (Constructor)

//
// Destructor
//
CVUMeterFilter::~CVUMeterFilter()
{
}

//
// DoRenderSample
//
// This is called when a sample is ready for rendering
//
HRESULT CVUMeterFilter::DoRenderSample(IMediaSample *pMediaSample)
{
	HRESULT						hr;
	REFERENCE_TIME				StartTime, StopTime;
	LPVUMeterCallbackStruc		lpVUMeterCBStruc;

	if (!pMediaSample)
	{
		// Skip for invalid sample
		return NOERROR;
	}

	// Allocate memory to store sample time and VU meter levels
	lpVUMeterCBStruc = (LPVUMeterCallbackStruc) GlobalAlloc(GPTR, sizeof(VUMeterCallbackStruc));

	if (!lpVUMeterCBStruc)
	{
		// Skip if not enough memory
		return NOERROR;
	}

	// Initialize structure
	lpVUMeterCBStruc->rtSampleStartTime = 0;
	lpVUMeterCBStruc->ulVUMeterLevels = 0;

	// Get media time for this sample
	hr = pMediaSample->GetTime(&StartTime, &StopTime);

	if (hr == S_OK || hr == VFW_S_NO_STOP_TIME)
	{
		// Okay, copy it into the structure
		lpVUMeterCBStruc->rtSampleStartTime = StartTime;
	}

	switch (m_lRawMediaType)
	{
		case RAW_VIDEO_MEDIA:
		{
			// Notify media time for video-only files
			NotifyEvent(EC_MEDIAPOS, 0, (LONG_PTR)lpVUMeterCBStruc);
			break;
		}

		case RAW_MIDI_MEDIA:
		{
			// Notify media time for MIDI files
			NotifyEvent(EC_MEDIAPOS, 0, (LONG_PTR)lpVUMeterCBStruc);
			break;
		}

		case RAW_AUDIO_MEDIA:
		{
			LPBYTE	pByte;

			// Notify media time and VU meter levels for media files with audio
			if (SUCCEEDED(pMediaSample->GetPointer(&pByte)))
			{
				long	lSumL, lSumR, lTmpVal;
				long	i, k;

			    long lDataLength = pMediaSample->GetActualDataLength();
				long eAudioFormat=UnknownFormat;

				// Currently support Microsoft PCM format only
				if (m_wfx.wFormatTag == 1)
				{
					// Microsoft PCM format
					if (m_wfx.nChannels == 1)
					{
						// MONO
						if (m_wfx.wBitsPerSample == 8)
						{
							eAudioFormat = Mono8;		// Mono 8bits
						}
						else if (m_wfx.wBitsPerSample == 16)
						{
							eAudioFormat = Mono16;		// Mono 16bits
						}
					}
					else if (m_wfx.nChannels == 2)
					{
						// Stereo
						if (m_wfx.wBitsPerSample == 8)
						{
							eAudioFormat = Stereo8;		// Stereo 8bits
						}
						else if (m_wfx.wBitsPerSample == 16)
						{
							eAudioFormat = Stereo16;	// Stereo 16bits
						}
					}

					k = lSumL = lSumR = 0;
					for (i=0; i<lDataLength; i+=64)
					{
						k++;

						switch (eAudioFormat)
						{
							case Mono8:
							{
								lSumL = lSumL + pow((128 - pByte[i]), 2);
								lSumR = lSumL;
								break;
							}

							case Stereo8:
							{
								lSumL = lSumL + pow((128 - pByte[i]), 2);
								lSumR = lSumR + pow((128 - pByte[i+1]), 2);
								break;
							}

							case Mono16:
							{
								lTmpVal = pByte[i+1] + pByte[i] / 256;
								if (lTmpVal > 127)
										lTmpVal -= 256;
								lSumL = lSumL + pow(lTmpVal, 2);
								lSumR = lSumL;
								break;
							}

							case Stereo16:
							{
								lTmpVal = pByte[i+1] + pByte[i] / 256;
								if (lTmpVal > 127) lTmpVal -= 256;
								lSumL = lSumL + pow(lTmpVal, 2);

								lTmpVal = pByte[i+3] + pByte[i+2] / 256;
								if (lTmpVal > 127) lTmpVal -= 256;
								lSumR = lSumR + pow(lTmpVal, 2);
								
								break;
							}

							default:
								break;
						}
					}

					lSumL = sqrt( lSumL / k );
					lSumR = sqrt( lSumR / k );

					if (lSumL > 100) lSumL = 100;
					if (lSumR > 100) lSumR = 100;

					lTmpVal = lSumL << 16;
					lTmpVal |= (lSumR & 0xFF);
					
					lpVUMeterCBStruc->ulVUMeterLevels = lTmpVal;
					NotifyEvent(EC_VUMETERLEVEL, m_wfx.nChannels, (LONG_PTR)lpVUMeterCBStruc);
				}
			}
			break;
		}
	}
    return NOERROR;

} // DoRenderSample

//
// CheckMediaType
//
// Check that we can support a given proposed type
//
HRESULT CVUMeterFilter::CheckMediaType(const CMediaType *pmt)
{
    // Reject non-interested type
    if (pmt->majortype != MEDIATYPE_Audio &&
		pmt->majortype != MEDIATYPE_Midi &&
		pmt->majortype != MEDIATYPE_Video)
	{
    	return E_INVALIDARG;
    }

    return NOERROR;

} // CheckMediaType

//
// SetMediaType
//
// Actually set the format of the input pin
//
HRESULT CVUMeterFilter::SetMediaType(const CMediaType *pmt)
{
    // Pass the call up to my base class
    HRESULT hr = CBaseRenderer::SetMediaType(pmt);
    if (SUCCEEDED(hr))
	{
		// Determine the source media type
		if (pmt->majortype == MEDIATYPE_Video)
		{
			// Video
			m_lRawMediaType = RAW_VIDEO_MEDIA;
		}
		else if (pmt->majortype == MEDIATYPE_Audio)
		{
			// Audio
			m_lRawMediaType = RAW_AUDIO_MEDIA;

			// For wave fomrat, store its properties...
			if (pmt->formattype == FORMAT_WaveFormatEx)
			{
				WAVEFORMATEX *pwf = (WAVEFORMATEX *) pmt->Format();

				m_wfx.wFormatTag = pwf->wFormatTag;
				m_wfx.nChannels = pwf->nChannels;
				m_wfx.nSamplesPerSec = pwf->nSamplesPerSec;
				m_wfx.nBlockAlign = pwf->nBlockAlign;
				m_wfx.wBitsPerSample = pwf->wBitsPerSample;
			}
		}
		else if (pmt->majortype == MEDIATYPE_Midi)
		{
			// Midi
			m_lRawMediaType = RAW_MIDI_MEDIA;
		}
		else
		{
			// Unknown
			m_lRawMediaType = RAW_UNSUPPORTED_MEDIA;
		}
    }
    return hr;

} // SetMediaType
