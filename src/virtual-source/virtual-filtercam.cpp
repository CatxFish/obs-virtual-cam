#include <streams.h>
#include "virtual-cam.h"
#include "virtual-filtercam.h"
#include <commctrl.h>
#include "resource.h"

CUnknown * WINAPI CVFilterCam::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
	ASSERT(phr);
	CUnknown *punk = new CVFilterCam(lpunk, phr, CLSID_OBS_VirtualFilter);
	return punk;
}

CVFilterCam::CVFilterCam(LPUNKNOWN lpunk, HRESULT *phr, const GUID id) :
CVCam(lpunk, phr, id)
{
	ReadRegisrty();
	SetQueueMode(filter_number);
}

HRESULT CVFilterCam::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	if (riid == IID_ISpecifyPropertyPages)
		return GetInterface(static_cast<ISpecifyPropertyPages*>(this), ppv);
	else if (riid == IID_IFilterSetting)
		return GetInterface(static_cast<IFilterSetting*>(this), ppv);

	return CVCam::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CVFilterCam::GetPages(CAUUID *pPages)
{
	if (pPages == NULL) 
		return E_POINTER;

	pPages->cElems = 1;
	pPages->pElems = (GUID*)CoTaskMemAlloc(sizeof(GUID));

	if (pPages->pElems == NULL)
		return E_OUTOFMEMORY;

	pPages->pElems[0] = CLSID_OBS_VirtualProp;
	return S_OK;
}
HRESULT CVFilterCam::GetQueueMode(int &mode)
{
	mode = filter_number;
	return S_OK;
}
HRESULT CVFilterCam::SetQueueMode(int mode)
{
	filter_number = mode;

	if (stream){
		stream->SetShareQueueMode(mode + ModeFilter1);
		return S_OK;
	}
	return E_FAIL;
}

bool CVFilterCam::ReadRegisrty()
{

	char* regname = "QueueMode";

	HKEY hkey;
	LPCTSTR sk = TEXT("Software\\OBS-VirtualCam\\");
	LONG openRes = RegOpenKeyEx(HKEY_CURRENT_USER, sk, 0, KEY_ALL_ACCESS, 
		&hkey);

	if (openRes == ERROR_SUCCESS) {
		DWORD value = 0x01;
		DWORD type = REG_DWORD;
		DWORD size = sizeof(value);
		openRes = RegQueryValueEx(hkey, regname, NULL, &type, (LPBYTE)&value, 
			&size);

		if (value >= 0 && value < 4 && openRes == ERROR_SUCCESS)
			filter_number = static_cast<int>(value);
		else {
			DWORD value = 0;
			long setRes = RegSetValueEx(hkey, regname, 0, REG_DWORD, 
				(LPBYTE)&value, sizeof(DWORD));
			filter_number = 0;
		}
	} else {
		DWORD dw;
		DWORD value = 0;
		if (RegCreateKeyEx(HKEY_CURRENT_USER, sk, 0, REG_NONE, 
			REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_READ, NULL, &hkey, &dw)
			== ERROR_SUCCESS) {
			long setRes = RegSetValueEx(hkey, regname, 0, REG_DWORD, 
				(LPBYTE)&value, sizeof(DWORD));
		}
		filter_number = 0;
	}

	RegCloseKey(hkey);
	return false;
}

bool CVFilterCam::WriteRegisrty()
{
	char* regname = "QueueMode";
	HKEY hKey;
	LPCTSTR sk = TEXT("Software\\OBS-VirtualCam\\");
	LONG openRes = RegOpenKeyEx(HKEY_CURRENT_USER, sk, 0, KEY_ALL_ACCESS, 
		&hKey);

	if (openRes == ERROR_SUCCESS) {
		DWORD value = static_cast<DWORD>(filter_number);
		RegSetValueEx(hKey, regname, NULL, REG_DWORD, (LPBYTE)&value, 
			sizeof(DWORD));
	}
	RegCloseKey(hKey);
	return openRes == ERROR_SUCCESS;
}

CFilterSetting::CFilterSetting(IUnknown *pUnk) : CBasePropertyPage(
	NAME("OBS-VirtualCam"), pUnk, IDD_DIALOG_FILTER, IDS_PROPPAGE_TITLE),
	filter_setting(0)
{
}

HRESULT CFilterSetting::OnConnect(IUnknown *pUnk)
{
	if (pUnk == NULL)
		return E_POINTER;
	ASSERT(filter_setting == NULL);
	return pUnk->QueryInterface(IID_IFilterSetting,
		reinterpret_cast<void**>(&filter_setting));
}

HRESULT CFilterSetting::OnActivate(void)
{
	INITCOMMONCONTROLSEX icc;
	icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icc.dwICC = ICC_BAR_CLASSES;
	if (InitCommonControlsEx(&icc) == FALSE)
		return E_FAIL;

	TCHAR source[4][12] = {
		TEXT("FilterCam-1"), TEXT("FilterCam-2"), 
		TEXT("FilterCam-3"), TEXT("FilterCam-4")
	};

	TCHAR A[12];
	memset(&A, 0, sizeof(A));

	for (int k = 0; k < 4; k++) {
		strcpy_s(A, sizeof(A) / sizeof(TCHAR), (TCHAR*)source[k]);
		SendDlgItemMessage(m_Dlg, IDC_COMBO, CB_ADDSTRING, 0, (LPARAM)A);
	}

	filter_setting->GetQueueMode(mode);
	SendDlgItemMessage(m_Dlg, IDC_COMBO, CB_SETCURSEL, (WPARAM)mode, 0);

	return S_OK;
}

INT_PTR CFilterSetting::OnReceiveMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_COMMAND:
		if (LOWORD(wParam) == IDC_COMBO && HIWORD(wParam) == CBN_SELCHANGE) {

			int selection = SendMessage(GetDlgItem(m_Dlg, IDC_COMBO), 
				CB_GETCURSEL, NULL, NULL);
			if (selection != CB_ERR) {
				mode = selection;
				SetDirty();
				return (LRESULT)1;
			}
		}
		break;
	}
	return CBasePropertyPage::OnReceiveMessage(hwnd, uMsg, wParam, lParam);
}

HRESULT CFilterSetting::OnApplyChanges(void)
{
	filter_setting->SetQueueMode(mode);
	return S_OK;
}


HRESULT CFilterSetting::OnDisconnect(void)
{
	return S_OK;
}