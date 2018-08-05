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

#define NUM_VIDEO_FILTERS 4

// {27B05C2D-93DC-474A-A5DA-9BBA34CB2A9C}
DEFINE_GUID(CLSID_OBS_VirtualV,
	0x27b05c2d, 0x93dc, 0x474a, 0xa5, 0xda, 0x9b, 0xba, 0x34, 0xcb, 0x2a, 0x9c);

DEFINE_GUID(CLSID_OBS_VirtualV2,
	0x27b05c2d, 0x93dc, 0x474a, 0xa5, 0xda, 0x9b, 0xba, 0x34, 0xcb, 0x2a, 0x9d);

DEFINE_GUID(CLSID_OBS_VirtualV3,
	0x27b05c2d, 0x93dc, 0x474a, 0xa5, 0xda, 0x9b, 0xba, 0x34, 0xcb, 0x2a, 0x9e);

DEFINE_GUID(CLSID_OBS_VirtualV4,
	0x27b05c2d, 0x93dc, 0x474a, 0xa5, 0xda, 0x9b, 0xba, 0x34, 0xcb, 0x2a, 0x9f);

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

const AMOVIESETUP_FILTER AMSFilterV2 =
{
	&CLSID_OBS_VirtualV2,
	L"OBS Virtual Cam2",
	MERIT_DO_NOT_USE,
	1,
	&AMSPinV
};

const AMOVIESETUP_FILTER AMSFilterV3 =
{
	&CLSID_OBS_VirtualV3,
	L"OBS Virtual Cam3",
	MERIT_DO_NOT_USE,
	1,
	&AMSPinV
};

const AMOVIESETUP_FILTER AMSFilterV4 =
{
	&CLSID_OBS_VirtualV4,
	L"OBS Virtual Cam4",
	MERIT_DO_NOT_USE,
	1,
	&AMSPinV
};

const AMOVIESETUP_FILTER AMSFilterA =
{
	&CLSID_OBS_VirtualA,  
	L"OBS Virtual Audio",     
	MERIT_DO_NOT_USE,      
	1,                     
	&AMSPinA             
};

CFactoryTemplate g_Templates[NUM_VIDEO_FILTERS + 1] =
{
	{
		L"OBS-Camera",
		&CLSID_OBS_VirtualV,
		CreateInstance,
		NULL,
		&AMSFilterV
	},
	{
		L"OBS-Camera2",
		&CLSID_OBS_VirtualV2,
		CreateInstance2,
		NULL,
		&AMSFilterV2
	},
	{
		L"OBS-Camera3",
		&CLSID_OBS_VirtualV3,
		CreateInstance3,
		NULL,
		&AMSFilterV3
	},
	{
		L"OBS-Camera4",
		&CLSID_OBS_VirtualV4,
		CreateInstance4,
		NULL,
		&AMSFilterV4
	},
	{
		L"OBS-Audio",
		&CLSID_OBS_VirtualA,
		CVAudio::CreateInstance,
		NULL,
		&AMSFilterA
	}
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

STDAPI RegisterFilters(BOOL bRegister,int reg_video_filters)
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

		hr = AMovieSetupRegisterServer(CLSID_OBS_VirtualA, L"OBS-Audio",
			achFileName);

		for (int i = 0; i < reg_video_filters; i++) {
			hr |= AMovieSetupRegisterServer(*(g_Templates[i].m_ClsID),
				g_Templates[i].m_Name, achFileName);
		}

	}

	if (SUCCEEDED(hr)) {

		IFilterMapper2 *fm = 0;
		hr = CreateComObject(CLSID_FilterMapper2, IID_IFilterMapper2, fm);

		if (SUCCEEDED(hr)) {
			if (bRegister) {
				IMoniker *moniker_audio = 0;
				REGFILTER2 rf2;
				rf2.dwVersion = 1;
				rf2.dwMerit = MERIT_DO_NOT_USE;
				rf2.cPins = 1;
				rf2.rgPins = &AMSPinA;
				hr = fm->RegisterFilter(CLSID_OBS_VirtualA, L"OBS-Audio",
					&moniker_audio, &CLSID_AudioInputDeviceCategory, NULL, &rf2);
				rf2.rgPins = &AMSPinV;
				for (int i = 0; i < reg_video_filters; i++) {
					IMoniker *moniker_video = 0;
					hr = fm->RegisterFilter(*(g_Templates[i].m_ClsID), 
						g_Templates[i].m_Name, &moniker_video, 
						&CLSID_VideoInputDeviceCategory, NULL, &rf2);
				}

			} else {
				hr = fm->UnregisterFilter(&CLSID_AudioInputDeviceCategory, 0, 
					CLSID_OBS_VirtualA);

				for (int i = 0; i < reg_video_filters; i++) {
					hr = fm->UnregisterFilter(&CLSID_VideoInputDeviceCategory, 0,
						*(g_Templates[i].m_ClsID));
				}
			}
		}

		if (fm)
			fm->Release();
	}

	if (SUCCEEDED(hr) && !bRegister){
		hr = AMovieSetupUnregisterServer(CLSID_OBS_VirtualA);
		for (int i = 0; i < reg_video_filters; i++) {
			hr = AMovieSetupUnregisterServer(*(g_Templates[i].m_ClsID));
		}
	}

	CoFreeUnusedLibraries();
	CoUninitialize();
	return hr;
}

STDAPI DllInstall(BOOL bInstall, _In_opt_ LPCWSTR pszCmdLine)
{
	if (!bInstall)
		return RegisterFilters(FALSE, NUM_VIDEO_FILTERS);
	else if (lstrcmpW(pszCmdLine, L"1") == 0)
		return RegisterFilters(TRUE, 1);
	else if (lstrcmpW(pszCmdLine, L"2") == 0)
		return RegisterFilters(TRUE, 2);
	else if (lstrcmpW(pszCmdLine, L"3") == 0)
		return RegisterFilters(TRUE, 3);
	else
		return RegisterFilters(TRUE, NUM_VIDEO_FILTERS);
}

STDAPI DllRegisterServer()
{
	return RegisterFilters(TRUE, NUM_VIDEO_FILTERS);
}

STDAPI DllUnregisterServer()
{
	return RegisterFilters(FALSE, NUM_VIDEO_FILTERS);
}

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, DWORD  dwReason, LPVOID lpReserved)
{
	return DllEntryPoint((HINSTANCE)(hModule), dwReason, lpReserved);
}
