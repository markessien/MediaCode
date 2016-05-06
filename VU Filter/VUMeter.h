//------------------------------------------------------------------------------
// File: VUMeter.h
//
// Desc: Header file for the VUMeterSample filter
//
// Copyright (c) 1997-2001 Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Define GUID for the VUMeterSample filter
//------------------------------------------------------------------------------
// {a87a43af-226d-4b85-92f4-bd19e92c914e}
DEFINE_GUID(CLSID_VUMeter, 
0xa87a43af, 0x226d, 0x4b85, 0x92, 0xf4, 0xbd, 0x19, 0xe9, 0x2c, 0x91, 0x4e);

#define EC_VUMETERLEVEL		EC_USER+1
#define EC_MEDIAPOS			EC_USER+2

#define RAW_UNSUPPORTED_MEDIA		0
#define RAW_AUDIO_MEDIA				1
#define RAW_VIDEO_MEDIA				2
#define RAW_MIDI_MEDIA				3

enum AUDIOFORMAT {UnknownFormat, Mono8, Stereo8, Mono16, Stereo16};

typedef struct tagCallbackStruc
{
	REFERENCE_TIME		rtSampleStartTime;
	ULONG				ulVUMeterLevels;
} VUMeterCallbackStruc, *LPVUMeterCallbackStruc;

// Overall filter object for the VUMeter renderer. We have to provide our own
// version of NonDelegatingQueryInterface so that we can expose not only the
// interfaces supported by the base renderer but also pass on queries for
// IVideoWindow to our window handling class (m_TextWindow). The rest of the
// methods we override are pretty dull, dealing with type checking and so on

class CVUMeterFilter : public CBaseRenderer
{
public:

    static CUnknown * WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *phr);

    CVUMeterFilter(LPUNKNOWN pUnk,HRESULT *phr);
    ~CVUMeterFilter();

    HRESULT CheckMediaType(const CMediaType *pmt);
	HRESULT SetMediaType(const CMediaType *pmt);
    HRESULT DoRenderSample(IMediaSample *pMediaSample);

private:
	WAVEFORMATEX	m_wfx;
	long			m_lRawMediaType;
}; // CVUMeterFilter
