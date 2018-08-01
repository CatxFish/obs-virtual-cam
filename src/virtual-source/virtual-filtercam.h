#pragma once

EXTERN_C const GUID CLSID_OBS_VirtualFilter;
EXTERN_C const GUID CLSID_OBS_VirtualProp;

// {1ABCC877-7131-407B-8F6B-44E5DB2E146C}
DEFINE_GUID(IID_IFilterSetting,
	0x1abcc877, 0x7131, 0x407b, 0x8f, 0x6b, 0x44, 0xe5, 0xdb, 0x2e, 0x14, 0x6c);


interface IFilterSetting : public IUnknown
{
	STDMETHOD(SetQueueMode)(int mode) = 0;
	STDMETHOD(GetQueueMode)(int &mode) = 0;
};


class CVFilterCam : public CVCam, public IFilterSetting, 
	public ISpecifyPropertyPages
{
public:
	DECLARE_IUNKNOWN;
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
	CVFilterCam(LPUNKNOWN lpunk, HRESULT *phr, const GUID id);
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);
	STDMETHODIMP GetPages(CAUUID *pPages);
	STDMETHODIMP GetQueueMode(int &mode);
	STDMETHODIMP SetQueueMode(int mode);
	bool ReadRegisrty();
	bool WriteRegisrty();
private:
	int filter_number;
};

class CFilterSetting : public CBasePropertyPage
{
private:
	IFilterSetting *filter_setting;    // Pointer to the filter's custom interface.
	int mode;

public:
	CFilterSetting(IUnknown *pUnk);
	HRESULT OnConnect(IUnknown *pUnk);
	HRESULT OnActivate(void);
	HRESULT OnDisconnect(void);
	HRESULT OnApplyChanges(void);
	INT_PTR OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void SetDirty() {
		m_bDirty = TRUE;
		if (m_pPageSite)
			m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
	}

	static CUnknown * WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT *pHr) {
		CFilterSetting *pNewObject = new CFilterSetting(pUnk);
		if (pNewObject == NULL)
			*pHr = E_OUTOFMEMORY;
		return pNewObject;
	}
};
