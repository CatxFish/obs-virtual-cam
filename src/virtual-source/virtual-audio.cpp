#pragma warning(disable:4244)
#pragma warning(disable:4711)

#include <streams.h>
#include <stdio.h>
#include <olectl.h>
#include <dvdmedia.h>
#include <commctrl.h>
#include "virtual-audio.h"

CUnknown * WINAPI CVAudio::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
	ASSERT(phr);
	CUnknown *punk = new CVAudio(lpunk, phr);
	return punk;
}

CVAudio::CVAudio(LPUNKNOWN lpunk, HRESULT *phr) : 
CSource(NAME("OBS Virtual Audio"), lpunk, CLSID_OBS_VirtualA)
{
	ASSERT(phr);
	CAutoLock cAutoLock(&m_cStateLock);
	m_paStreams = (CSourceStream **) new CVAudioStream*[1];
	m_paStreams[0] = new CVAudioStream(phr, this, L"Audio");
}

HRESULT CVAudio::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	if (riid == _uuidof(IAMStreamConfig) || riid == _uuidof(IKsPropertySet))
		return m_paStreams[0]->QueryInterface(riid, ppv);
	else if (riid == IID_IAMFilterMiscFlags)
		return GetInterface((IAMFilterMiscFlags*) this, ppv);
	else
		return CSource::NonDelegatingQueryInterface(riid, ppv);
}

CVAudioStream::CVAudioStream(HRESULT *phr, CVAudio *pParent, LPCWSTR pPinName) :
CSourceStream(NAME("Audio"), phr, pParent, pPinName), parent(pParent)
{

	m_allocProp.cBuffers = m_allocProp.cbBuffer = -1;
	m_allocProp.cbAlign = m_allocProp.cbPrefix = -1;
	GetMediaType(&m_mt);
}

CVAudioStream::~CVAudioStream()
{
}

HRESULT CVAudioStream::ChangeMediaType(int nMediatype)
{
	GetMediaType(&m_mt);
	return S_OK;
}

HRESULT CVAudioStream::QueryInterface(REFIID riid, void **ppv)
{
	if (riid == _uuidof(IAMStreamConfig))
		*ppv = (IAMStreamConfig*)this;
	else if (riid == _uuidof(IKsPropertySet))
		*ppv = (IKsPropertySet*)this;
	else if (riid == IID_IAMBufferNegotiation)
		*ppv = (IAMBufferNegotiation*)this;
	else
		return CSourceStream::QueryInterface(riid, ppv);
	AddRef();
	return S_OK;
}


HRESULT CVAudioStream::FillBuffer(IMediaSample *pms)
{
	HRESULT hr;
	bool get_sample = false;
	int get_times = 0;
	REFERENCE_TIME start_time = 0;
	REFERENCE_TIME end_time = 0;
	uint8_t* dst;
	uint32_t size = 0;
	uint64_t timestamp = 0;

	hr = pms->GetPointer((BYTE**)&dst);

	if (!queue.hwnd)
		shared_queue_open(&queue, ModeAudio);

	while (queue.header && !get_sample){

		if (get_times > 20 || queue.header->state != OutputReady)
			break;

		get_sample = shared_queue_get_audio(&queue, dst, AUDIO_BUFFER_SIZE, &timestamp);

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
		size = queue.operating_width;
		start_time = dshow_start_ts + (timestamp - obs_start_ts) / 100;
		pms->SetActualDataLength(size);
	}else{
		start_time = prev_end_ts;
		size = pms->GetActualDataLength();
		memset(dst, 0, size);
	}

	if (queue.header && queue.header->state == OutputStop || get_times > 20){
		shared_queue_read_close(&queue,NULL);
		obs_start_ts = 0;
		dshow_start_ts = 0;
	}

	if (prev_end_ts != start_time)
		int a = 0;

	REFERENCE_TIME duration = (REFERENCE_TIME)10000000 * size / 
		(REFERENCE_TIME)SAMPLE_SIZE;
	end_time = start_time + duration;
	prev_end_ts = end_time;
	


	pms->SetTime(&start_time, &end_time);
	pms->SetSyncPoint(TRUE);
	return NOERROR;
}


STDMETHODIMP CVAudioStream::Notify(IBaseFilter * pSender, Quality q)
{
	return E_NOTIMPL;
} 


HRESULT CVAudioStream::Stop()
{
	return S_OK;
}

HRESULT CVAudioStream::Pause()
{
	return S_OK;
}

HRESULT CVAudioStream::SetMediaType(const CMediaType *pmt)
{

	DECLARE_PTR(WAVEFORMATEX, paf, pmt->Format());

	HRESULT hr = CSourceStream::SetMediaType(pmt);
	return hr;
}

HRESULT CVAudioStream::GetMediaType(CMediaType *pmt)
{
	DECLARE_PTR(WAVEFORMATEX, paf, pmt->AllocFormatBuffer(sizeof(WAVEFORMATEX)));
	ZeroMemory(paf, sizeof(WAVEFORMATEX));

	paf->nChannels = 2;
	paf->nSamplesPerSec = SAMPLE_RATE;
	paf->nAvgBytesPerSec = SAMPLE_SIZE;
	paf->nBlockAlign = 4;
	paf->wBitsPerSample = 16;
	paf->cbSize = 0;
	paf->wFormatTag = WAVE_FORMAT_PCM;

	HRESULT hr = ::CreateAudioMediaType(paf, pmt, FALSE);

	return hr;
} 

HRESULT CVAudioStream::CheckMediaType(const CMediaType *pMediaType)
{
	WAVEFORMATEX *paf = (WAVEFORMATEX *)(pMediaType->Format());
	if (*pMediaType != m_mt)
		return E_INVALIDARG;

	return S_OK;
}

HRESULT CVAudioStream::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProperties)
{
	HRESULT hr = S_OK;
	CAutoLock cAutoLock(m_pFilter->pStateLock());

	CheckPointer(pAlloc, E_POINTER);
	CheckPointer(pProperties, E_POINTER);

	WAVEFORMATEX* pwfex = (WAVEFORMATEX*)m_mt.Format();
	if (pwfex){

		pProperties->cBuffers = 1;
		pProperties->cbBuffer = AUDIO_BUFFER_SIZE;

		if (0 < m_allocProp.cBuffers)
			pProperties->cBuffers = m_allocProp.cBuffers;

		if (0 < m_allocProp.cbBuffer)
			pProperties->cbBuffer = m_allocProp.cbBuffer;

		if (0 < m_allocProp.cbAlign)
			pProperties->cbAlign = m_allocProp.cbAlign;

		if (0 < m_allocProp.cbPrefix)
			pProperties->cbPrefix = m_allocProp.cbPrefix;

		ALLOCATOR_PROPERTIES Actual;
		hr = pAlloc->SetProperties(pProperties, &Actual);
		if (SUCCEEDED(hr))
			if (Actual.cbBuffer < pProperties->cbBuffer)
				hr = E_FAIL;

		
	}else{
		hr = E_POINTER;
	}
	return hr;
} 

HRESULT CVAudioStream::OnThreadCreate()
{
	obs_start_ts = 0;
	dshow_start_ts = 0;
	prev_end_ts = 0;
	return NOERROR;
} 

HRESULT CVAudioStream::OnThreadDestroy()
{
	if (queue.header)
		shared_queue_read_close(&queue,NULL);
	return NOERROR;
}


HRESULT STDMETHODCALLTYPE CVAudioStream::SetFormat(AM_MEDIA_TYPE *pmt)
{
	if (CheckMediaType((CMediaType *)pmt) != S_OK) {
		return E_FAIL;
	}

	IPin* pin;
	ConnectedTo(&pin);
	if (pin)
	{
		IFilterGraph *pGraph = parent->GetGraph();
		pGraph->Reconnect(this);
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CVAudioStream::GetFormat(AM_MEDIA_TYPE **ppmt)
{
	*ppmt = CreateMediaType(&m_mt);
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CVAudioStream::GetNumberOfCapabilities(int *piCount, int *piSize)
{
	*piCount = 1;
	*piSize = sizeof(AUDIO_STREAM_CONFIG_CAPS);
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CVAudioStream::GetStreamCaps(int iIndex, AM_MEDIA_TYPE **pmt, BYTE *pSCC)
{
	*pmt = CreateMediaType(&m_mt);
	DECLARE_PTR(WAVEFORMATEX, paf, (*pmt)->pbFormat);

	paf->nChannels = 2;
	paf->nSamplesPerSec = SAMPLE_RATE;
	paf->nAvgBytesPerSec = SAMPLE_SIZE;
	paf->nBlockAlign = 4;
	paf->wBitsPerSample = 16;
	paf->cbSize = 0;
	paf->wFormatTag = WAVE_FORMAT_PCM;

	HRESULT hr = ::CreateAudioMediaType(paf, *pmt, FALSE);
	DECLARE_PTR(AUDIO_STREAM_CONFIG_CAPS, pascc, pSCC);

	pascc->guid = MEDIATYPE_Audio;
	pascc->BitsPerSampleGranularity = 16;
	pascc->ChannelsGranularity = 2;
	pascc->MaximumBitsPerSample = 16;
	pascc->MaximumChannels = 2;
	pascc->MaximumSampleFrequency = SAMPLE_RATE;
	pascc->MinimumBitsPerSample = 16;
	pascc->MinimumChannels = 2;
	pascc->MinimumSampleFrequency = SAMPLE_RATE;
	pascc->SampleFrequencyGranularity = SAMPLE_RATE;


	return hr;
}

STDMETHODIMP CVAudioStream::SuggestAllocatorProperties(const ALLOCATOR_PROPERTIES* pprop)
{
	HRESULT hr = S_OK;
	if (pprop){
		if (IsConnected())
			hr = VFW_E_ALREADY_CONNECTED;
		else
			m_allocProp = *pprop;
	}else
		hr = E_POINTER;
	
	return hr;
}

STDMETHODIMP CVAudioStream::GetAllocatorProperties(ALLOCATOR_PROPERTIES* pprop)
{
	HRESULT hr = S_OK;
	if (pprop){
		if (IsConnected())
			*pprop = m_allocProp;
		else
			hr = VFW_E_NOT_CONNECTED;
	}
	else
		hr = E_POINTER;
	return hr;
}

HRESULT CVAudioStream::Set(REFGUID guidPropSet, DWORD dwID, void *pInstanceData,
	DWORD cbInstanceData, void *pPropData, DWORD cbPropData)
{
	return E_NOTIMPL;
}

HRESULT CVAudioStream::Get(REFGUID guidPropSet,DWORD dwPropID,
	void *pInstanceData,DWORD cbInstanceData, void *pPropData,       
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

HRESULT CVAudioStream::QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD *pTypeSupport)
{
	if (guidPropSet != AMPROPSETID_Pin) return E_PROP_SET_UNSUPPORTED;
	if (dwPropID != AMPROPERTY_PIN_CATEGORY) return E_PROP_ID_UNSUPPORTED;
	if (pTypeSupport) *pTypeSupport = KSPROPERTY_SUPPORT_GET;
	return S_OK;
}
