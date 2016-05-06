// CDROM utilities function to enable/disable W2K/XP "Enable Digital Playback" hardware setting
#include "stdafx.h"
#include <Setupapi.h>
#include <winioctl.h>

void YieldControl();

DEFINE_GUID(GUID_DEVINTERFACE_CDROM,
0x53f56308L, 0xb6bf, 0x11d0, 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b);

#define DEFINEMY_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        const GUID name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

DEFINEMY_GUID(cdromGUID, 0x53f56308L, 0xb6bf, 0x11d0, 0x94, 0xf2, 0x00, 0xa0, 0xc9, 0x1e, 0xfb, 0x8b);
   

typedef struct _SP_DEVICE_INTERFACE_DETAIL_DATA_MINE
{
   DWORD cbSize;
   CHAR  DevicePath[255];
} SP_DEVICE_INTERFACE_DETAIL_DATA_MINE, * PSP_DEVICE_INTERFACE_DETAIL_DATA_MINE;

void DisableCDROMDigitalPlayback(char *szDrive)
{
	TCHAR									szCmdLine[512];
	HINSTANCE 								hInstSetupDLL, hInstStorpropDLL;
	HDEVINFO								DeviceInfoSet;
	SP_DEVICE_INTERFACE_DATA				DeviceInterfaceData;
	SP_DEVICE_INTERFACE_DETAIL_DATA_MINE	DeviceInterfaceDetailData;
	SP_DEVINFO_DATA							DeviceInfoData;
   	DWORD									Err, dwDevices, dwReturnCode=0;

	HDEVINFO (__stdcall *MySetupDiGetClassDevsEx)(CONST GUID *, PCSTR, HWND, DWORD, HDEVINFO, PCSTR, PVOID);
	BOOL (__stdcall *MySetupDiEnumDeviceInterfaces)(HDEVINFO, PSP_DEVINFO_DATA, CONST GUID *,
											DWORD, PSP_DEVICE_INTERFACE_DATA);
	BOOL (__stdcall *MySetupDiGetDeviceInterfaceDetail)(HDEVINFO, PSP_DEVICE_INTERFACE_DATA,
												PSP_DEVICE_INTERFACE_DETAIL_DATA_A,
												DWORD, PDWORD, PSP_DEVINFO_DATA);
	BOOL (__stdcall *MySetupDiDestroyDeviceInfoList)(HDEVINFO);
	LONG (__stdcall *MyCdromIsDigitalPlaybackEnabled)(HDEVINFO, PSP_DEVINFO_DATA, PBOOLEAN);
	BOOL (__stdcall *MySetupDiGetDeviceRegistryProperty)(HDEVINFO, PSP_DEVINFO_DATA, DWORD, PDWORD, PBYTE, DWORD, PDWORD);

	typedef HDEVINFO (__stdcall *fpType1)(CONST GUID *, PCSTR, HWND, DWORD, HDEVINFO, PCSTR, PVOID);
	typedef BOOL (__stdcall *fpType2)(HDEVINFO, PSP_DEVINFO_DATA, CONST GUID *, DWORD, PSP_DEVICE_INTERFACE_DATA);
	typedef BOOL (__stdcall *fpType3)(HDEVINFO, PSP_DEVICE_INTERFACE_DATA, PSP_DEVICE_INTERFACE_DETAIL_DATA_A, DWORD, PDWORD, PSP_DEVINFO_DATA);
	typedef BOOL (__stdcall *fpType4)(HDEVINFO);
	typedef LONG (__stdcall *fpType5)(HDEVINFO, PSP_DEVINFO_DATA, PBOOLEAN);
	typedef BOOL (__stdcall *fpType6)(HDEVINFO, PSP_DEVINFO_DATA, DWORD, PDWORD, PBYTE, DWORD, PDWORD);

	//
	// Detect which OS we are running on. For Windows 2000 & XP, use the storprop.dll;
	// for Windows 95/98/NT platform, skip this function...
	//
	OSVERSIONINFO 	ver;
	ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&ver);
	if (VER_PLATFORM_WIN32_NT == ver.dwPlatformId)
	{
		if (ver.dwMajorVersion <= 4)
		{
			// Windows 9x/Me/NT, Nothing to do
			return;
		}
	}

	GetSystemDirectory(szCmdLine, MAX_PATH);
	lstrcat(szCmdLine, TEXT("\\setupapi.dll"));

	hInstSetupDLL = LoadLibrary(szCmdLine);
	if (hInstSetupDLL)
	{
		MySetupDiGetClassDevsEx = (fpType1)GetProcAddress(hInstSetupDLL, "SetupDiGetClassDevsExA");
		MySetupDiEnumDeviceInterfaces = (fpType2)GetProcAddress(hInstSetupDLL, "SetupDiEnumDeviceInterfaces");
		MySetupDiGetDeviceInterfaceDetail = (fpType3)GetProcAddress(hInstSetupDLL, "SetupDiGetDeviceInterfaceDetailA");
		MySetupDiDestroyDeviceInfoList = (fpType4)GetProcAddress(hInstSetupDLL, "SetupDiDestroyDeviceInfoList");
		MySetupDiGetDeviceRegistryProperty = (fpType6)GetProcAddress(hInstSetupDLL, "SetupDiGetDeviceRegistryPropertyA");

		if (!MySetupDiGetClassDevsEx || !MySetupDiEnumDeviceInterfaces
			|| !MySetupDiGetDeviceInterfaceDetail || !MySetupDiDestroyDeviceInfoList)
		{
			return;
		}

		// Windows 2000/XP

		// Retrieve the device information set for the interface class.
		DeviceInfoSet = MySetupDiGetClassDevsEx((LPGUID)&cdromGUID,
			NULL,
			NULL,
			DIGCF_PRESENT | DIGCF_DEVICEINTERFACE,
			NULL,
			NULL,
			NULL
		);

		if(DeviceInfoSet == INVALID_HANDLE_VALUE) 
		{
			Err = GetLastError();
			return;
		}

		dwDevices = 0;

		while (!dwReturnCode)
		{
			ZeroMemory(&DeviceInterfaceData, sizeof(SP_DEVICE_INTERFACE_DATA));
			ZeroMemory(&DeviceInterfaceDetailData, sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA));
			ZeroMemory(&DeviceInfoData, sizeof(SP_DEVINFO_DATA));
			DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
			DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
			
			if (!MySetupDiEnumDeviceInterfaces(DeviceInfoSet, NULL, (LPGUID)&cdromGUID,
										dwDevices, &DeviceInterfaceData))
			{
				dwReturnCode = GetLastError();

				dwDevices++;
				continue;
			}

			DeviceInterfaceDetailData.cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
			
			if (MySetupDiGetDeviceInterfaceDetail(
											   DeviceInfoSet,
											   &DeviceInterfaceData,
											   (PSP_DEVICE_INTERFACE_DETAIL_DATA) &DeviceInterfaceDetailData,
											   255,
											   NULL,
											   &DeviceInfoData))
			{
				UCHAR	buf[128];
				DWORD	dwSize;

				GetSystemDirectory(szCmdLine, MAX_PATH);
				lstrcat(szCmdLine, TEXT("\\storprop.dll"));

				hInstStorpropDLL = LoadLibrary(szCmdLine);
				if (hInstStorpropDLL)
				{
					MyCdromIsDigitalPlaybackEnabled = (fpType5)GetProcAddress(hInstStorpropDLL, "CdromIsDigitalPlaybackEnabled");

					if (MyCdromIsDigitalPlaybackEnabled)
					{
						BOOLEAN		bEnabled;

						MyCdromIsDigitalPlaybackEnabled(DeviceInfoSet, &DeviceInfoData, &bEnabled);

						if (bEnabled == TRUE)
						{
							SetupDiGetDeviceRegistryProperty(DeviceInfoSet, &DeviceInfoData, 0x11, 0, 0, 0, &dwSize);
							SetupDiGetDeviceRegistryProperty(DeviceInfoSet, &DeviceInfoData, 0x11, 0, buf, dwSize, &dwSize);

							if (!lstrcmpi("redbook", (LPCSTR)buf))
							{
								if (SetupDiSetDeviceRegistryProperty(DeviceInfoSet, &DeviceInfoData, 0x11, 0, 0))
								{
									Sleep(5000);
									YieldControl();

									SP_PROPCHANGE_PARAMS	PropParams;

									PropParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
									PropParams.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
									PropParams.StateChange = DICS_STOP;
									PropParams.Scope = DICS_FLAG_CONFIGSPECIFIC;
									PropParams.HwProfile = 0;

									SetupDiSetClassInstallParams(DeviceInfoSet, &DeviceInfoData,
										(PSP_CLASSINSTALL_HEADER)&PropParams, sizeof(SP_PROPCHANGE_PARAMS));

									SetupDiCallClassInstaller(DIF_PROPERTYCHANGE, DeviceInfoSet, &DeviceInfoData);

									Sleep(5000);
									YieldControl();

									PropParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
									PropParams.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
									PropParams.StateChange = DICS_START;
									PropParams.Scope = DICS_FLAG_CONFIGSPECIFIC;
									PropParams.HwProfile = 0;

									SetupDiSetClassInstallParams(DeviceInfoSet, &DeviceInfoData,
										(PSP_CLASSINSTALL_HEADER)&PropParams, sizeof(SP_PROPCHANGE_PARAMS));

									SetupDiCallClassInstaller(DIF_PROPERTYCHANGE, DeviceInfoSet, &DeviceInfoData);

									Sleep(5000);
									YieldControl();

									SP_DEVINSTALL_PARAMS  DeviceInstallParams;

									DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
									SetupDiGetDeviceInstallParams(DeviceInfoSet, &DeviceInfoData, &DeviceInstallParams);
								}
							}
						}
					}
					FreeLibrary(hInstStorpropDLL);

				}
			}
			dwDevices++;
		}
		
		SetupDiDestroyDeviceInfoList(DeviceInfoSet);

		FreeLibrary(hInstSetupDLL);
	}
}

void EnableCDROMDigitalPlayback(char *szDrive)
{
	TCHAR									szCmdLine[512];
	HINSTANCE 								hInstSetupDLL, hInstStorpropDLL;
	HDEVINFO								DeviceInfoSet;
	SP_DEVICE_INTERFACE_DATA				DeviceInterfaceData;
	SP_DEVICE_INTERFACE_DETAIL_DATA_MINE	DeviceInterfaceDetailData;
	SP_DEVINFO_DATA							DeviceInfoData;
   	DWORD									Err, dwDevices, dwReturnCode=0;

	HDEVINFO (__stdcall *MySetupDiGetClassDevsEx)(CONST GUID *, PCSTR, HWND, DWORD, HDEVINFO, PCSTR, PVOID);
	BOOL (__stdcall *MySetupDiEnumDeviceInterfaces)(HDEVINFO, PSP_DEVINFO_DATA, CONST GUID *,
											DWORD, PSP_DEVICE_INTERFACE_DATA);
	BOOL (__stdcall *MySetupDiGetDeviceInterfaceDetail)(HDEVINFO, PSP_DEVICE_INTERFACE_DATA,
												PSP_DEVICE_INTERFACE_DETAIL_DATA_A,
												DWORD, PDWORD, PSP_DEVINFO_DATA);
	BOOL (__stdcall *MySetupDiDestroyDeviceInfoList)(HDEVINFO);
	LONG (__stdcall *MyCdromIsDigitalPlaybackEnabled)(HDEVINFO, PSP_DEVINFO_DATA, PBOOLEAN);
	BOOL (__stdcall *MySetupDiGetDeviceRegistryProperty)(HDEVINFO, PSP_DEVINFO_DATA, DWORD, PDWORD, PBYTE, DWORD, PDWORD);

	typedef HDEVINFO (__stdcall *fpType1)(CONST GUID *, PCSTR, HWND, DWORD, HDEVINFO, PCSTR, PVOID);
	typedef BOOL (__stdcall *fpType2)(HDEVINFO, PSP_DEVINFO_DATA, CONST GUID *, DWORD, PSP_DEVICE_INTERFACE_DATA);
	typedef BOOL (__stdcall *fpType3)(HDEVINFO, PSP_DEVICE_INTERFACE_DATA, PSP_DEVICE_INTERFACE_DETAIL_DATA_A, DWORD, PDWORD, PSP_DEVINFO_DATA);
	typedef BOOL (__stdcall *fpType4)(HDEVINFO);
	typedef LONG (__stdcall *fpType5)(HDEVINFO, PSP_DEVINFO_DATA, PBOOLEAN);
	typedef BOOL (__stdcall *fpType6)(HDEVINFO, PSP_DEVINFO_DATA, DWORD, PDWORD, PBYTE, DWORD, PDWORD);

	//
	// Detect which OS we are running on. For Windows 2000 & XP, use the storprop.dll;
	// for Windows 95/98/NT platform, skip this function...
	//
	OSVERSIONINFO 	ver;
	ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&ver);
	if (VER_PLATFORM_WIN32_NT == ver.dwPlatformId)
	{
		if (ver.dwMajorVersion <= 4)
		{
			// Windows 9x/Me/NT, Nothing to do
			return;
		}
	}

	GetSystemDirectory(szCmdLine, MAX_PATH);
	lstrcat(szCmdLine, TEXT("\\setupapi.dll"));

	hInstSetupDLL = LoadLibrary(szCmdLine);
	if (hInstSetupDLL)
	{
		MySetupDiGetClassDevsEx = (fpType1)GetProcAddress(hInstSetupDLL, "SetupDiGetClassDevsExA");
		MySetupDiEnumDeviceInterfaces = (fpType2)GetProcAddress(hInstSetupDLL, "SetupDiEnumDeviceInterfaces");
		MySetupDiGetDeviceInterfaceDetail = (fpType3)GetProcAddress(hInstSetupDLL, "SetupDiGetDeviceInterfaceDetailA");
		MySetupDiDestroyDeviceInfoList = (fpType4)GetProcAddress(hInstSetupDLL, "SetupDiDestroyDeviceInfoList");
		MySetupDiGetDeviceRegistryProperty = (fpType6)GetProcAddress(hInstSetupDLL, "SetupDiGetDeviceRegistryPropertyA");

		if (!MySetupDiGetClassDevsEx || !MySetupDiEnumDeviceInterfaces
			|| !MySetupDiGetDeviceInterfaceDetail || !MySetupDiDestroyDeviceInfoList)
		{
			return;
		}

		// Windows 2000/XP

		// Retrieve the device information set for the interface class.
		DeviceInfoSet = MySetupDiGetClassDevsEx((LPGUID)&cdromGUID,
			NULL,
			NULL,
			DIGCF_PRESENT | DIGCF_DEVICEINTERFACE,
			NULL,
			NULL,
			NULL
		);

		if(DeviceInfoSet == INVALID_HANDLE_VALUE) 
		{
			Err = GetLastError();
			return;
		}

		dwDevices = 0;

		while (!dwReturnCode)
		{
			ZeroMemory(&DeviceInterfaceData, sizeof(SP_DEVICE_INTERFACE_DATA));
			ZeroMemory(&DeviceInterfaceDetailData, sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA));
			ZeroMemory(&DeviceInfoData, sizeof(SP_DEVINFO_DATA));
			DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
			DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
			
			if (!MySetupDiEnumDeviceInterfaces(DeviceInfoSet, NULL, (LPGUID)&cdromGUID,
										dwDevices, &DeviceInterfaceData))
			{
				dwReturnCode = GetLastError();

				dwDevices++;
				continue;
			}

			DeviceInterfaceDetailData.cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
			
			if (MySetupDiGetDeviceInterfaceDetail(
											   DeviceInfoSet,
											   &DeviceInterfaceData,
											   (PSP_DEVICE_INTERFACE_DETAIL_DATA) &DeviceInterfaceDetailData,
											   255,
											   NULL,
											   &DeviceInfoData))
			{
				UCHAR	buf[128];
				DWORD	dwSize;

				GetSystemDirectory(szCmdLine, MAX_PATH);
				lstrcat(szCmdLine, TEXT("\\storprop.dll"));

				hInstStorpropDLL = LoadLibrary(szCmdLine);
				if (hInstStorpropDLL)
				{
					MyCdromIsDigitalPlaybackEnabled = (fpType5)GetProcAddress(hInstStorpropDLL, "CdromIsDigitalPlaybackEnabled");

					if (MyCdromIsDigitalPlaybackEnabled)
					{
						BOOLEAN		bEnabled;

						MyCdromIsDigitalPlaybackEnabled(DeviceInfoSet, &DeviceInfoData, &bEnabled);

						if (bEnabled == FALSE)
						{
							SetupDiGetDeviceRegistryProperty(DeviceInfoSet, &DeviceInfoData, 0x11, 0, 0, 0, &dwSize);
							SetupDiGetDeviceRegistryProperty(DeviceInfoSet, &DeviceInfoData, 0x11, 0, buf, dwSize, &dwSize);

							lstrcpy((LPSTR)buf, "redbook");
							SetupDiSetDeviceRegistryProperty(DeviceInfoSet, &DeviceInfoData, 0x11, buf, lstrlen((LPSTR)buf));

							SP_PROPCHANGE_PARAMS	PropParams;

							PropParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
							PropParams.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
							PropParams.StateChange = DICS_STOP;
							PropParams.Scope = DICS_FLAG_CONFIGSPECIFIC;
							PropParams.HwProfile = 0;

							SetupDiSetClassInstallParams(DeviceInfoSet, &DeviceInfoData,
								(PSP_CLASSINSTALL_HEADER)&PropParams, sizeof(SP_PROPCHANGE_PARAMS));

							SetupDiCallClassInstaller(DIF_PROPERTYCHANGE, DeviceInfoSet, &DeviceInfoData);

							PropParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
							PropParams.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
							PropParams.StateChange = DICS_START;
							PropParams.Scope = DICS_FLAG_CONFIGSPECIFIC;
							PropParams.HwProfile = 0;

							SetupDiSetClassInstallParams(DeviceInfoSet, &DeviceInfoData,
								(PSP_CLASSINSTALL_HEADER)&PropParams, sizeof(SP_PROPCHANGE_PARAMS));

							SetupDiCallClassInstaller(DIF_PROPERTYCHANGE, DeviceInfoSet, &DeviceInfoData);

							SP_DEVINSTALL_PARAMS  DeviceInstallParams;

							DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
							SetupDiGetDeviceInstallParams(DeviceInfoSet, &DeviceInfoData, &DeviceInstallParams);
						}
					}
					FreeLibrary(hInstStorpropDLL);

				}
			}
			dwDevices++;
		}
		
		SetupDiDestroyDeviceInfoList(DeviceInfoSet);

		FreeLibrary(hInstSetupDLL);
	}
}

BOOL GetCDROMDeviceInfo(HDEVINFO *DeviceInfoSet, PSP_DEVINFO_DATA pDeviceInfoData)
{
	TCHAR									szCmdLine[512];
	HINSTANCE 								hInstSetupDLL;
	HDEVINFO								MyDeviceInfoSet;
	SP_DEVICE_INTERFACE_DATA				DeviceInterfaceData;
	SP_DEVICE_INTERFACE_DETAIL_DATA_MINE	DeviceInterfaceDetailData;
	SP_DEVINFO_DATA							DeviceInfoData;
	BOOL									bReturn=FALSE;
   	DWORD									dwDevices;

	HDEVINFO (__stdcall *MySetupDiGetClassDevsEx)(CONST GUID *, PCSTR, HWND, DWORD, HDEVINFO, PCSTR, PVOID);
	BOOL (__stdcall *MySetupDiEnumDeviceInterfaces)(HDEVINFO, PSP_DEVINFO_DATA, CONST GUID *,
											DWORD, PSP_DEVICE_INTERFACE_DATA);
	BOOL (__stdcall *MySetupDiGetDeviceInterfaceDetail)(HDEVINFO, PSP_DEVICE_INTERFACE_DATA,
												PSP_DEVICE_INTERFACE_DETAIL_DATA_A,
												DWORD, PDWORD, PSP_DEVINFO_DATA);
	BOOL (__stdcall *MySetupDiDestroyDeviceInfoList)(HDEVINFO);

	typedef HDEVINFO (__stdcall *fpType1)(CONST GUID *, PCSTR, HWND, DWORD, HDEVINFO, PCSTR, PVOID);
	typedef BOOL (__stdcall *fpType2)(HDEVINFO, PSP_DEVINFO_DATA, CONST GUID *, DWORD, PSP_DEVICE_INTERFACE_DATA);
	typedef BOOL (__stdcall *fpType3)(HDEVINFO, PSP_DEVICE_INTERFACE_DATA, PSP_DEVICE_INTERFACE_DETAIL_DATA_A, DWORD, PDWORD, PSP_DEVINFO_DATA);
	typedef BOOL (__stdcall *fpType4)(HDEVINFO);

	GetSystemDirectory(szCmdLine, MAX_PATH);
	lstrcat(szCmdLine, TEXT("\\setupapi.dll"));

	hInstSetupDLL = LoadLibrary(szCmdLine);
	if (hInstSetupDLL)
	{
		MySetupDiGetClassDevsEx = (fpType1)GetProcAddress(hInstSetupDLL, "SetupDiGetClassDevsExA");
		MySetupDiEnumDeviceInterfaces = (fpType2)GetProcAddress(hInstSetupDLL, "SetupDiEnumDeviceInterfaces");
		MySetupDiGetDeviceInterfaceDetail = (fpType3)GetProcAddress(hInstSetupDLL, "SetupDiGetDeviceInterfaceDetailA");
		MySetupDiDestroyDeviceInfoList = (fpType4)GetProcAddress(hInstSetupDLL, "SetupDiDestroyDeviceInfoList");

		if (!MySetupDiGetClassDevsEx || !MySetupDiEnumDeviceInterfaces
			|| !MySetupDiGetDeviceInterfaceDetail || !MySetupDiDestroyDeviceInfoList)
		{
			return bReturn;
		}

		// Windows 2000/XP

		// Retrieve the device information set for the interface class.
		MyDeviceInfoSet = MySetupDiGetClassDevsEx((LPGUID)&cdromGUID,
			NULL,
			NULL,
			DIGCF_PRESENT | DIGCF_DEVICEINTERFACE,
			NULL,
			NULL,
			NULL
		);

		if (MyDeviceInfoSet == INVALID_HANDLE_VALUE) 
		{
			return bReturn;
		}

		dwDevices = 0;
		
		ZeroMemory(&DeviceInterfaceData, sizeof(SP_DEVICE_INTERFACE_DATA));
		ZeroMemory(&DeviceInterfaceDetailData, sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA));
		ZeroMemory(&DeviceInfoData, sizeof(SP_DEVINFO_DATA));
		DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
		DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
		
		if (MySetupDiEnumDeviceInterfaces(MyDeviceInfoSet, NULL, (LPGUID)&cdromGUID,
									dwDevices, &DeviceInterfaceData))
		{
			DeviceInterfaceDetailData.cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
			
			if (MySetupDiGetDeviceInterfaceDetail(
											   MyDeviceInfoSet,
											   &DeviceInterfaceData,
											   (PSP_DEVICE_INTERFACE_DETAIL_DATA) &DeviceInterfaceDetailData,
											   255,
											   NULL,
											   &DeviceInfoData))
			{
				*DeviceInfoSet = MyDeviceInfoSet;
				CopyMemory(pDeviceInfoData, &DeviceInfoData, sizeof(SP_DEVINFO_DATA));
				bReturn = TRUE;
			}
		}

		FreeLibrary(hInstSetupDLL);
	}
	return bReturn;
}

void YieldControl()
{
	MSG msg;

	while (PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE))
	{
		if (msg.message == WM_QUIT || msg.message == WM_CLOSE)
		{
			break;
		}

		if (GetMessage(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}