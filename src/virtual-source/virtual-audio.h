#pragma once

#include "../queue/share_queue_read.h"

#define DECLARE_PTR(type, ptr, expr) type* ptr = (type*)(expr);

#define AUDIO_BUFFER_SIZE 4096  
#define SAMPLE_RATE 44100
#define SAMPLE_SIZE 176400

EXTERN_C const GUID CLSID_OBS_VirtualA;

class CVAudioStream;
class CVAudio : public CSource, public  IAMFilterMiscFlags
{
public:
	DECLARE_IUNKNOWN;
	//////////////////////////////////////////////////////////////////////////
	//  IUnknown
	//////////////////////////////////////////////////////////////////////////
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);
	//////////////////////////////////////////////////////////////////////////
	//  IAMFilterMiscFlags
	//////////////////////////////////////////////////////////////////////////	

	virtual ULONG STDMETHODCALLTYPE GetMiscFlags(void)
	{
		return AM_FILTER_MISC_FLAGS_IS_SOURCE;
	}

	IFilterGraph *GetGraph() { return m_pGraph; }
	CVAudio(LPUNKNOWN lpunk, HRESULT *phr);
};


class CVAudioStream : public CSourceStream, public IAMStreamConfig, 
	public IKsPropertySet, public IAMBufferNegotiation
{
public:

	//////////////////////////////////////////////////////////////////////////
	//  IUnknown
	//////////////////////////////////////////////////////////////////////////
	STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
	STDMETHODIMP_(ULONG) AddRef() { return GetOwner()->AddRef(); }
	STDMETHODIMP_(ULONG) Release() { return GetOwner()->Release(); }

	//////////////////////////////////////////////////////////////////////////
	//  IQualityControl
	//////////////////////////////////////////////////////////////////////////
	STDMETHODIMP Notify(IBaseFilter * pSender, Quality q);

	//////////////////////////////////////////////////////////////////////////
	//  IAMStreamConfig
	//////////////////////////////////////////////////////////////////////////
	HRESULT STDMETHODCALLTYPE SetFormat(AM_MEDIA_TYPE *pmt);
	HRESULT STDMETHODCALLTYPE GetFormat(AM_MEDIA_TYPE **ppmt);
	HRESULT STDMETHODCALLTYPE GetNumberOfCapabilities(int *piCount, int *piSize);
	HRESULT STDMETHODCALLTYPE GetStreamCaps(int iIndex, 
		AM_MEDIA_TYPE **pmt, BYTE *pSCC);

	//////////////////////////////////////////////////////////////////////////
	//  IKsPropertySet
	//////////////////////////////////////////////////////////////////////////
	HRESULT STDMETHODCALLTYPE Set(REFGUID guidPropSet, DWORD dwID, 
		void *pInstanceData, DWORD cbInstanceData, void *pPropData, 
		DWORD cbPropData);
	HRESULT STDMETHODCALLTYPE Get(REFGUID guidPropSet, DWORD dwPropID, 
		void *pInstanceData, DWORD cbInstanceData, void *pPropData, 
		DWORD cbPropData, DWORD *pcbReturned);
	HRESULT STDMETHODCALLTYPE QuerySupported(REFGUID guidPropSet, 
		DWORD dwPropID, DWORD *pTypeSupport);

	//////////////////////////////////////////////////////////////////////////
	// IAMBufferNegotiation interface
	//////////////////////////////////////////////////////////////////////////
	STDMETHODIMP SuggestAllocatorProperties(const ALLOCATOR_PROPERTIES* pprop);
	STDMETHODIMP GetAllocatorProperties(ALLOCATOR_PROPERTIES* pprop);


	//////////////////////////////////////////////////////////////////////////
	//  CSourceStream
	//////////////////////////////////////////////////////////////////////////
	CVAudioStream(HRESULT *phr, CVAudio *pParent, LPCWSTR pPinName);
	~CVAudioStream();

	HRESULT FillBuffer(IMediaSample *pms);
	HRESULT DecideBufferSize(IMemAllocator *pIMemAlloc, ALLOCATOR_PROPERTIES *pProperties);
	HRESULT CheckMediaType(const CMediaType *pMediaType);
	HRESULT GetMediaType(CMediaType *pmt);
	HRESULT SetMediaType(const CMediaType *pmt);
	HRESULT OnThreadCreate(void);
	HRESULT OnThreadDestroy(void);
	HRESULT ChangeMediaType(int nMediatype);
	HRESULT Stop(void);
	HRESULT Pause(void);


private:
	void SetTimeout();

	CVAudio *parent;
	share_queue queue;
	uint64_t obs_start_ts = 0;
	uint64_t dshow_start_ts = 0;
	uint64_t sync_timeout = 0;
	uint64_t system_start_time = 0;
	REFERENCE_TIME  prev_end_ts = 0;
	ALLOCATOR_PROPERTIES alloc_prop;

};