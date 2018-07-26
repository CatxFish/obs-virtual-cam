#include <streams.h>
#include <initguid.h>
#include "virtual-cam.h"
#include "virtual-audio.h"

#define CreateComObject(clsid, iid, var) CoCreateInstance(clsid, NULL, \
CLSCTX_INPROC_SERVER, iid, (void **)&var);

STDAPI AMovieSetupRegisterServer(CLSID clsServer, LPCWSTR szDescription, 
	LPCWSTR szFileName, LPCWSTR szThreadingModel = L"Both", 
	LPCWSTR szServerType = L"InprocServer32");
STDAPI AMovieSetupUnregisterServer(CLSID clsServer);



// {27B05C2D-93DC-474A-A5DA-9BBA34CB2A9C}
DEFINE_GUID(CLSID_OBS_VirtualV,
	0x27b05c2d, 0x93dc, 0x474a, 0xa5, 0xda, 0x9b, 0xba, 0x34, 0xcb, 0x2a, 0x9c);

// {B750E5CD-5E7E-4ED3-B675-A5003C439997}
DEFINE_GUID(CLSID_OBS_VirtualA,
	0xb750e5cd, 0x5e7e, 0x4ed3, 0xb6, 0x75, 0xa5, 0x0, 0x3c, 0x43, 0x99, 0x97);



const AMOVIESETUP_MEDIATYPE AMSMediaTypesV =
{
	&MEDIATYPE_Video,
	&MEDIASUBTYPE_YUY2
};

const AMOVIESETUP_MEDIATYPE AMSMediaTypesA =
{
	&MEDIATYPE_Audio,
	&MEDIASUBTYPE_PCM
};

const AMOVIESETUP_PIN AMSPinV =
{
	L"Output",            
	FALSE,                 
	TRUE,                  
	FALSE,                 
	FALSE,                 
	&CLSID_NULL,           
	NULL,                  
	1,                     
	&AMSMediaTypesV
};

const AMOVIESETUP_PIN AMSPinA =
{
	L"Output",             
	FALSE,                 
	TRUE,                  
	FALSE,                 
	FALSE,                 
	&CLSID_NULL,           
	NULL,                  
	1,                     
	&AMSMediaTypesA
};

const AMOVIESETUP_FILTER AMSFilterV =
{
	&CLSID_OBS_VirtualV,  
	L"OBS Virtual Cam",     
	MERIT_DO_NOT_USE,      
	1,                     
	&AMSPinV
};

const AMOVIESETUP_FILTER AMSFilterA =
{
	&CLSID_OBS_VirtualA,  // Filter CLSID
	L"OBS Virtual Audio",     // String name
	MERIT_DO_NOT_USE,      // Filter merit
	1,                     // Number pins
	&AMSPinA             // Pin details
};

CFactoryTemplate g_Templates[] =
{
	{
		L"OBS Virtual Cam",
		&CLSID_OBS_VirtualV,
		CVCam::CreateInstance,
		NULL,
		&AMSFilterV
	},
	{
		L"OBS Virtual Audio",
		&CLSID_OBS_VirtualA,
		CVAudio::CreateInstance,
		NULL,
		&AMSFilterA
	},
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

void RegisterDummyDevicePath()
{
	HKEY hKey;
	std::string str_video_capture_device_key(
		"SOFTWARE\\Classes\\CLSID\\{860BB310-5D01-11d0-BD3B-00A0C911CE86}\\Instance\\");

	LPOLESTR olestr_CLSID;
	StringFromCLSID(CLSID_OBS_VirtualV, &olestr_CLSID);

	std::wstring wstr_CLSID(olestr_CLSID);

	int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr_CLSID[0], 
		(int)wstr_CLSID.size(), NULL, 0, NULL, NULL);
	std::string str2(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, &wstr_CLSID[0], (int)wstr_CLSID.size(), 
		&str2[0], size_needed, NULL, NULL);

	str_video_capture_device_key.append(str2);
	
	RegOpenKeyEx(HKEY_LOCAL_MACHINE, str_video_capture_device_key.c_str(), 0,
		KEY_ALL_ACCESS, &hKey);
		
	LPCTSTR value = TEXT("DevicePath");
	LPCTSTR data = "obs:virtualcam";
	RegSetValueEx(hKey, value, 0, REG_SZ, (LPBYTE)data, strlen(data) + 1);
	RegCloseKey(hKey);
}

STDAPI RegisterFilters(BOOL bRegister)
{
	HRESULT hr = NOERROR;
	WCHAR achFileName[MAX_PATH];
	char achTemp[MAX_PATH];
	ASSERT(g_hInst != 0);

	if (0 == GetModuleFileNameA(g_hInst, achTemp, sizeof(achTemp)))
		return AmHresultFromWin32(GetLastError());

	MultiByteToWideChar(CP_ACP, 0L, achTemp, lstrlenA(achTemp) + 1,
		achFileName, NUMELMS(achFileName));

	hr = CoInitialize(0);
	if (bRegister) {
		hr = AMovieSetupRegisterServer(CLSID_OBS_VirtualV, L"OBS-Camera", 
			achFileName, L"Both", L"InprocServer32");
		hr |= AMovieSetupRegisterServer(CLSID_OBS_VirtualA, L"OBS-Audio",
			achFileName, L"Both", L"InprocServer32");
	}

	if (SUCCEEDED(hr)) {

		IFilterMapper2 *fm = 0;
		hr = CreateComObject(CLSID_FilterMapper2, IID_IFilterMapper2, fm);

		if (SUCCEEDED(hr)) {
			if (bRegister) {
				IMoniker *moniker_video = 0, *moniker_audio = 0;
				REGFILTER2 rf2;
				rf2.dwVersion = 1;
				rf2.dwMerit = MERIT_DO_NOT_USE;
				rf2.cPins = 1;
				rf2.rgPins = &AMSPinV;
				hr = fm->RegisterFilter(CLSID_OBS_VirtualV, L"OBS-Camera", 
					&moniker_video, &CLSID_VideoInputDeviceCategory, NULL, &rf2);
				rf2.rgPins = &AMSPinA;
				hr = fm->RegisterFilter(CLSID_OBS_VirtualA, L"OBS-Audio", 
					&moniker_audio, &CLSID_AudioInputDeviceCategory, NULL, &rf2);

			} else {
				hr = fm->UnregisterFilter(&CLSID_AudioInputDeviceCategory, 0, CLSID_OBS_VirtualA);
				hr = fm->UnregisterFilter(&CLSID_VideoInputDeviceCategory, 0, CLSID_OBS_VirtualV);
			}
		}

		// Dummy DevicePath conflicts with Skype
		/*if (SUCCEEDED(hr) && bRegister)
			RegisterDummyDevicePath();*/

		if (fm)
			fm->Release();
	}

	if (SUCCEEDED(hr) && !bRegister){
		hr = AMovieSetupUnregisterServer(CLSID_OBS_VirtualA);
		hr = AMovieSetupUnregisterServer(CLSID_OBS_VirtualV);
	}

	CoFreeUnusedLibraries();
	CoUninitialize();
	return hr;
}

STDAPI DllRegisterServer()
{
	return RegisterFilters(TRUE);
}

STDAPI DllUnregisterServer()
{
	return RegisterFilters(FALSE);
}

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, DWORD  dwReason, LPVOID lpReserved)
{
	return DllEntryPoint((HINSTANCE)(hModule), dwReason, lpReserved);
}
