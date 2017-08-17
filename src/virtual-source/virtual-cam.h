#pragma once

extern "C"
{
#include "libswscale/swscale.h" 
};

#include "../queue/share_queue_read.h"

#define DECLARE_PTR(type, ptr, expr) type* ptr = (type*)(expr);

EXTERN_C const GUID CLSID_OBS_VirtualV;

class CVCamStream;

class CVCam : public CSource
{
public:
	DECLARE_IUNKNOWN;
	//////////////////////////////////////////////////////////////////////////
	//  IUnknown
	//////////////////////////////////////////////////////////////////////////
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);
	IFilterGraph *GetGraph() { return m_pGraph; }
	FILTER_STATE GetState(){ return m_State; }
	CVCam(LPUNKNOWN lpunk, HRESULT *phr);

};

class CVCamStream : public CSourceStream, public IAMStreamConfig,public IKsPropertySet
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
	HRESULT STDMETHODCALLTYPE GetStreamCaps(int iIndex, AM_MEDIA_TYPE **pmt,
		BYTE *pSCC);

	//////////////////////////////////////////////////////////////////////////
	//  IKsPropertySet
	//////////////////////////////////////////////////////////////////////////
	HRESULT STDMETHODCALLTYPE Set(REFGUID guidPropSet, DWORD dwID, 
		void *pInstanceData, DWORD cbInstanceData, void *pPropData, DWORD cbPropData);

	HRESULT STDMETHODCALLTYPE Get(REFGUID guidPropSet, DWORD dwPropID, 
		void *pInstanceData, DWORD cbInstanceData, void *pPropData, 
		DWORD cbPropData, DWORD *pcbReturned);

	HRESULT STDMETHODCALLTYPE QuerySupported(REFGUID guidPropSet, 
		DWORD dwPropID, DWORD *pTypeSupport);

	//////////////////////////////////////////////////////////////////////////
	//  CSourceStream
	//////////////////////////////////////////////////////////////////////////
	CVCamStream(HRESULT *phr, CVCam *pParent, LPCWSTR pPinName);
	~CVCamStream();

	HRESULT FillBuffer(IMediaSample *pms);
	HRESULT DecideBufferSize(IMemAllocator *pIMemAlloc, ALLOCATOR_PROPERTIES *pProperties);
	HRESULT CheckMediaType(const CMediaType *pMediaType);
	HRESULT GetMediaType(int iPosition,CMediaType *pmt);
	HRESULT SetMediaType(const CMediaType *pmt);
	HRESULT OnThreadCreate(void);
	HRESULT OnThreadDestroy(void);
	HRESULT ChangeMediaType(int nMediatype);
	bool CheckObsSetting();
	bool ValidateResolution(long width, long height);
	void SetConvertContext();


private:
	CVCam *parent;
	REFERENCE_TIME  prev_end_ts =0;
	share_queue queue = {};
	uint64_t obs_start_ts = 0;
	uint64_t dshow_start_ts = 0;
	uint8_t* dst;
	int format = 0;
	int frame_width = 0;
	int frame_height = 0;
	int64_t time_perframe = 0;

	bool use_obs_format_init = false;
	int obs_format = 0;
	int obs_width = 1920;
	int obs_height = 1080;
	int64_t obs_frame_time = 333333;
	dst_scale_context scale_info;
};
