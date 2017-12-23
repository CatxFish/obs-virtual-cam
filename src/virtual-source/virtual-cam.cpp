#include <streams.h>
#include <stdio.h>
#include <olectl.h>
#include <dvdmedia.h>
#include <commctrl.h>
#include "virtual-cam.h"

#define MIN_WIDTH 320
#define MIN_HEIGHT 240
#define MAX_WIDTH 4096
#define MAX_HEIGHT 3072
#define MAX_FRAMETIME 1000000
#define MIN_FRAMETIME 166666


CUnknown * WINAPI CVCam::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
	ASSERT(phr);
	CUnknown *punk = new CVCam(lpunk, phr);
	return punk;
}

CVCam::CVCam(LPUNKNOWN lpunk, HRESULT *phr) :
CSource(NAME("OBS Virtual CAM"), lpunk, CLSID_OBS_VirtualV)
{
	ASSERT(phr);
	CAutoLock cAutoLock(&m_cStateLock);
	m_paStreams = (CSourceStream **) new CVCamStream*[1];
	m_paStreams[0] = new CVCamStream(phr, this, L"Video");
}

HRESULT CVCam::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	if (riid == _uuidof(IAMStreamConfig) || riid == _uuidof(IKsPropertySet))
		return m_paStreams[0]->QueryInterface(riid, ppv);
	else
		return CSource::NonDelegatingQueryInterface(riid, ppv);
}

CVCamStream::CVCamStream(HRESULT *phr, CVCam *pParent, LPCWSTR pPinName) :
CSourceStream(NAME("Video"), phr, pParent, pPinName), parent(pParent)
{
	ListSupportFormat();
	use_obs_format_init = CheckObsSetting();
	GetMediaType(0,&m_mt);
	prev_end_ts = 0;
}

CVCamStream::~CVCamStream()
{
}

bool CVCamStream::CheckObsSetting()
{
	bool get= shared_queue_get_video_format(&obs_format, &obs_width,
		&obs_height, &obs_frame_time);


	if (get){
		if (obs_frame_time < MIN_FRAMETIME || obs_frame_time > MAX_FRAMETIME)
			return false;

		if (obs_height < MIN_HEIGHT){
			obs_width = obs_width * MIN_HEIGHT / obs_height;
			obs_height = MIN_HEIGHT;
		}

		if (obs_width < MIN_WIDTH){
			obs_height = obs_height * MIN_WIDTH / obs_width;
			obs_width = MIN_WIDTH;
		}

		if (obs_height % 2 != 0)
			obs_height += 1;

		format_list.push_front(struct format(obs_width,obs_height, obs_frame_time));
	}

	return get;
}

void CVCamStream::SetConvertContext()
{
	VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *)(m_mt.Format());
	scale_info.dst_format = AV_PIX_FMT_YUYV422;
	scale_info.dst_width = pvi->bmiHeader.biWidth;
	scale_info.dst_height = pvi->bmiHeader.biHeight;
	scale_info.dst_linesize[0] = pvi->bmiHeader.biWidth * 2;
}

HRESULT CVCamStream::QueryInterface(REFIID riid, void **ppv)
{
	if (riid == _uuidof(IAMStreamConfig))
		*ppv = (IAMStreamConfig*)this;
	else if (riid == _uuidof(IKsPropertySet))
		*ppv = (IKsPropertySet*)this;
	else
		return CSourceStream::QueryInterface(riid, ppv);

	AddRef();
	return S_OK;
}

HRESULT CVCamStream::FillBuffer(IMediaSample *pms)
{
	HRESULT hr;
	bool get_sample = false;
	int get_times = 0;
	uint64_t timestamp=0;
	REFERENCE_TIME start_time=0;
	REFERENCE_TIME end_time = 0;
	REFERENCE_TIME duration = 0;

	hr = pms->GetPointer((BYTE**)&dst);

	if (!queue.hwnd){
		if (shared_queue_open(&queue, ModeVideo)){
			shared_queue_get_video_format(&format, &frame_width, 
				&frame_height, &time_perframe);
			SetConvertContext();
		}
	}

	while (queue.header && !get_sample){

		if (get_times > 20 || queue.header->state != OutputReady)
			break;

		get_sample = shared_queue_get_video(&queue, &scale_info,dst,&timestamp);

		if (!get_sample){
			Sleep(5);
			get_times++;
		}
	} 
	
	if (get_sample && !obs_start_ts){
		obs_start_ts = timestamp;
		dshow_start_ts = prev_end_ts;
	}

	if (get_sample){
		start_time = dshow_start_ts + (timestamp - obs_start_ts) / 100;
		duration = time_perframe;
	}else{
		int size = pms->GetActualDataLength();
		memset(dst, 127, size);
		start_time = prev_end_ts;
		duration = ((VIDEOINFOHEADER*)m_mt.pbFormat)->AvgTimePerFrame;
	}

	if (queue.header && queue.header->state == OutputStop || get_times > 20){
		shared_queue_read_close(&queue,&scale_info);
		dshow_start_ts = 0;
		obs_start_ts = 0;
	}

	end_time = start_time + duration;
	prev_end_ts = end_time;
	pms->SetTime(&start_time, &end_time);
	pms->SetSyncPoint(TRUE);

	return NOERROR;
}

STDMETHODIMP CVCamStream::Notify(IBaseFilter * pSender, Quality q)
{
	return E_NOTIMPL;
}

bool CVCamStream::ListSupportFormat()
{
	if (format_list.size() > 0)
		format_list.empty();
	
	format_list.push_back(struct format(1920, 1080, 333333));
	format_list.push_back(struct format(1280, 720, 333333));
	format_list.push_back(struct format(960, 540, 333333));
	format_list.push_back(struct format(640, 360, 333333));

	return true;
}

HRESULT CVCamStream::SetMediaType(const CMediaType *pmt)
{
	DECLARE_PTR(VIDEOINFOHEADER, pvi, pmt->Format());
	HRESULT hr = CSourceStream::SetMediaType(pmt);
	return hr;
}

HRESULT CVCamStream::GetMediaType(int iPosition,CMediaType *pmt)
{

	if (format_list.size() == 0)
		ListSupportFormat();

	if (iPosition < 0 || iPosition > format_list.size()-1)
		return E_INVALIDARG;

	DECLARE_PTR(VIDEOINFOHEADER, pvi, 
	pmt->AllocFormatBuffer(sizeof(VIDEOINFOHEADER)));
	ZeroMemory(pvi, sizeof(VIDEOINFOHEADER));


	pvi->bmiHeader.biWidth = format_list[iPosition].width;
	pvi->bmiHeader.biHeight = format_list[iPosition].height;
	pvi->AvgTimePerFrame = format_list[iPosition].time_per_frame;
	pvi->bmiHeader.biCompression = MAKEFOURCC('Y', 'U', 'Y', '2');;
	pvi->bmiHeader.biBitCount = 16;
	pvi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pvi->bmiHeader.biPlanes = 1;
	pvi->bmiHeader.biSizeImage = pvi->bmiHeader.biWidth * 
		pvi->bmiHeader.biHeight * 2;
	pvi->bmiHeader.biClrImportant = 0;

	SetRectEmpty(&(pvi->rcSource)); 
	SetRectEmpty(&(pvi->rcTarget)); 

	pmt->SetType(&MEDIATYPE_Video);
	pmt->SetFormatType(&FORMAT_VideoInfo);
	pmt->SetTemporalCompression(FALSE);
	pmt->SetSubtype(&MEDIASUBTYPE_YUY2);
	pmt->SetSampleSize(pvi->bmiHeader.biSizeImage);
	return NOERROR;

} 

HRESULT CVCamStream::CheckMediaType(const CMediaType *pMediaType)
{
	if (pMediaType == nullptr)
		return E_FAIL;

	VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *)(pMediaType->Format());

	const GUID* type = pMediaType->Type();
	const GUID* info = pMediaType->FormatType();
	const GUID* subtype = pMediaType->Subtype();

	if (*type != MEDIATYPE_Video)
		return E_INVALIDARG;

	if (*info != FORMAT_VideoInfo)
		return E_INVALIDARG;

	if (*subtype != MEDIASUBTYPE_YUY2)
		return E_INVALIDARG;

	if (pvi->AvgTimePerFrame < 166666 || pvi->AvgTimePerFrame >1000000)
		return E_INVALIDARG;

	if (ValidateResolution(pvi->bmiHeader.biWidth, pvi->bmiHeader.biHeight))
		return S_OK;

	return E_INVALIDARG;
} 

bool CVCamStream::ValidateResolution(long width,long height)
{
	if (width < 320 || height <240)
		return false;
	else if (width > 4096)
		return false;
	else if (width * 9 == height * 16)
		return true;
	else if (width * 3 == height * 4)
		return true;
	else if (use_obs_format_init && width == obs_width && height == obs_height)
		return true;
	else
		return false;
}

HRESULT CVCamStream::DecideBufferSize(IMemAllocator *pAlloc, 
	ALLOCATOR_PROPERTIES *pProperties)
{
	CAutoLock cAutoLock(m_pFilter->pStateLock());
	HRESULT hr = NOERROR;

	VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *)m_mt.Format();
	pProperties->cBuffers = 1;
	pProperties->cbBuffer = pvi->bmiHeader.biSizeImage;

	ALLOCATOR_PROPERTIES Actual;
	hr = pAlloc->SetProperties(pProperties, &Actual);

	if (FAILED(hr)) return hr;
	if (Actual.cbBuffer < pProperties->cbBuffer) return E_FAIL;

	return NOERROR;
} 

HRESULT CVCamStream::OnThreadCreate()
{
	prev_end_ts = 0;
	obs_start_ts = 0;
	dshow_start_ts = 0;
	return NOERROR;
}

HRESULT CVCamStream::OnThreadDestroy()
{
	if (queue.header){
		shared_queue_read_close(&queue, &scale_info);
	}
	return NOERROR;
}

HRESULT STDMETHODCALLTYPE CVCamStream::SetFormat(AM_MEDIA_TYPE *pmt)
{
	if (pmt == nullptr)
		return E_FAIL;

	if (parent->GetState() != State_Stopped)
		return E_FAIL;

	if (CheckMediaType((CMediaType *)pmt) != S_OK)
		return E_FAIL;

	VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *)(pmt->pbFormat);

	m_mt.SetFormat(m_mt.Format(), sizeof(VIDEOINFOHEADER));
	format_list.push_front(struct format(pvi->bmiHeader.biWidth, 
		pvi->bmiHeader.biHeight, pvi->AvgTimePerFrame));

	IPin* pin;
	ConnectedTo(&pin);
	if (pin){
		IFilterGraph *pGraph = parent->GetGraph();
		pGraph->Reconnect(this);
	}
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CVCamStream::GetFormat(AM_MEDIA_TYPE **ppmt)
{
	*ppmt = CreateMediaType(&m_mt);
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CVCamStream::GetNumberOfCapabilities(int *piCount, 
	int *piSize)
{
	*piCount = 4;
	if (use_obs_format_init)
		*piCount = *piCount + 1;

	*piSize = sizeof(VIDEO_STREAM_CONFIG_CAPS);
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CVCamStream::GetStreamCaps(int iIndex, 
	AM_MEDIA_TYPE **pmt, BYTE *pSCC)
{
	if (format_list.size() == 0)
		ListSupportFormat();

	if (iIndex < 0 || iIndex > format_list.size() - 1)
		return E_INVALIDARG;

	*pmt = CreateMediaType(&m_mt);
	DECLARE_PTR(VIDEOINFOHEADER, pvi, (*pmt)->pbFormat);


	pvi->bmiHeader.biWidth = format_list[iIndex].width;
	pvi->bmiHeader.biHeight = format_list[iIndex].height;
	pvi->AvgTimePerFrame = format_list[iIndex].time_per_frame;
	pvi->AvgTimePerFrame = 333333;
	pvi->bmiHeader.biCompression = MAKEFOURCC('Y', 'U', 'Y', '2');;
	pvi->bmiHeader.biBitCount = 16;
	pvi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pvi->bmiHeader.biPlanes = 1;
	pvi->bmiHeader.biSizeImage = pvi->bmiHeader.biWidth * 
		pvi->bmiHeader.biHeight * 2;
	pvi->bmiHeader.biClrImportant = 0;

	SetRectEmpty(&(pvi->rcSource)); 
	SetRectEmpty(&(pvi->rcTarget)); 

	(*pmt)->majortype = MEDIATYPE_Video;
	(*pmt)->subtype = MEDIASUBTYPE_YUY2;
	(*pmt)->formattype = FORMAT_VideoInfo;
	(*pmt)->bTemporalCompression = FALSE;
	(*pmt)->bFixedSizeSamples = FALSE;
	(*pmt)->lSampleSize = pvi->bmiHeader.biSizeImage;
	(*pmt)->cbFormat = sizeof(VIDEOINFOHEADER);

	DECLARE_PTR(VIDEO_STREAM_CONFIG_CAPS, pvscc, pSCC);

	pvscc->guid = FORMAT_VideoInfo;
	pvscc->VideoStandard = AnalogVideo_None;
	pvscc->InputSize.cx = pvi->bmiHeader.biWidth;
	pvscc->InputSize.cy = pvi->bmiHeader.biHeight;
	pvscc->MinCroppingSize.cx = pvi->bmiHeader.biWidth;
	pvscc->MinCroppingSize.cy = pvi->bmiHeader.biHeight;
	pvscc->MaxCroppingSize.cx = pvi->bmiHeader.biWidth;
	pvscc->MaxCroppingSize.cy = pvi->bmiHeader.biHeight;
	pvscc->CropGranularityX = pvi->bmiHeader.biWidth;
	pvscc->CropGranularityY = pvi->bmiHeader.biHeight;
	pvscc->CropAlignX = 0;
	pvscc->CropAlignY = 0;

	pvscc->MinOutputSize.cx = pvi->bmiHeader.biWidth;
	pvscc->MinOutputSize.cy = pvi->bmiHeader.biHeight;
	pvscc->MaxOutputSize.cx = pvi->bmiHeader.biWidth;
	pvscc->MaxOutputSize.cy = pvi->bmiHeader.biHeight;
	pvscc->OutputGranularityX = 0;
	pvscc->OutputGranularityY = 0;
	pvscc->StretchTapsX = 0;
	pvscc->StretchTapsY = 0;
	pvscc->ShrinkTapsX = 0;
	pvscc->ShrinkTapsY = 0;
	pvscc->MinFrameInterval = pvi->AvgTimePerFrame;   
	pvscc->MaxFrameInterval = pvi->AvgTimePerFrame; 
	pvscc->MinBitsPerSecond = pvi->bmiHeader.biWidth * pvi->bmiHeader.biHeight 
		* 2 * 8 * (10000000 / pvi->AvgTimePerFrame);
	pvscc->MaxBitsPerSecond = pvi->bmiHeader.biWidth * pvi->bmiHeader.biHeight 
		* 2 * 8 * (10000000 / pvi->AvgTimePerFrame);

	return S_OK;
}

HRESULT CVCamStream::Set(REFGUID guidPropSet, DWORD dwID, void *pInstanceData,
	DWORD cbInstanceData, void *pPropData, DWORD cbPropData)
{
	return E_NOTIMPL;
}

HRESULT CVCamStream::Get(REFGUID guidPropSet,DWORD dwPropID,
	void *pInstanceData,DWORD cbInstanceData,void *pPropData,
	DWORD cbPropData,DWORD *pcbReturned)
{
	if (guidPropSet != AMPROPSETID_Pin)             return E_PROP_SET_UNSUPPORTED;
	if (dwPropID != AMPROPERTY_PIN_CATEGORY)        return E_PROP_ID_UNSUPPORTED;
	if (pPropData == NULL && pcbReturned == NULL)   return E_POINTER;

	if (pcbReturned) *pcbReturned = sizeof(GUID);
	if (pPropData == NULL)          return S_OK; 
	if (cbPropData < sizeof(GUID))  return E_UNEXPECTED;

	*(GUID *)pPropData = PIN_CATEGORY_CAPTURE;
	return S_OK;
}

HRESULT CVCamStream::QuerySupported(REFGUID guidPropSet, DWORD dwPropID, 
	DWORD *pTypeSupport)
{
	if (guidPropSet != AMPROPSETID_Pin) return E_PROP_SET_UNSUPPORTED;
	if (dwPropID != AMPROPERTY_PIN_CATEGORY) return E_PROP_ID_UNSUPPORTED;
	if (pTypeSupport) *pTypeSupport = KSPROPERTY_SUPPORT_GET;
	return S_OK;
}

