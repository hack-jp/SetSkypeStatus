﻿#pragma comment(lib,"wtsapi32")
#include <windows.h>
#include <wtsapi32.h>

TCHAR szClassName[] = TEXT("SetSkypeStatus");

DWORD_PTR SendSkypeMessage(HWND hWnd, HWND hGlobal_SkypeAPIWindowHandle, LPCSTR szMessage)
{
	COPYDATASTRUCT oCopyData;
	oCopyData.dwData = 0;
	oCopyData.lpData = (void*)szMessage;
	oCopyData.cbData = lstrlenA(szMessage) + 1;
	return SendMessage(hGlobal_SkypeAPIWindowHandle, WM_COPYDATA, (WPARAM)hWnd, (LPARAM)&oCopyData);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static HWND hGlobal_SkypeAPIWindowHandle = 0;
	static UINT uiGlobal_MsgID_SkypeControlAPIAttach = 0;
	static UINT uiGlobal_MsgID_SkypeControlAPIDiscover = 0;
	switch (msg)
	{
	case WM_COPYDATA:
		if (hGlobal_SkypeAPIWindowHandle == (HWND)wParam)
		{
#ifdef _DEBUG
			COPYDATASTRUCT*pCopyData = (COPYDATASTRUCT*)lParam;
			LPSTR lpszText = (LPSTR)GlobalAlloc(0, pCopyData->cbData);
			lstrcpyA(lpszText, (LPSTR)(pCopyData->lpData));
			OutputDebugStringA(lpszText);
			OutputDebugString(TEXT("\r\n"));
#endif
			return 1;
		}
		break;
	case WM_CREATE:
		uiGlobal_MsgID_SkypeControlAPIAttach = RegisterWindowMessageA("SkypeControlAPIAttach");
		uiGlobal_MsgID_SkypeControlAPIDiscover = RegisterWindowMessageA("SkypeControlAPIDiscover");
		if (uiGlobal_MsgID_SkypeControlAPIAttach == 0 || uiGlobal_MsgID_SkypeControlAPIDiscover == 0)return -1;

		while (!hGlobal_SkypeAPIWindowHandle)
		{
			SendMessage(HWND_BROADCAST, uiGlobal_MsgID_SkypeControlAPIDiscover, (WPARAM)hWnd, 0);
			Sleep(10 * 1000);

			MSG msg;
			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

		PostMessage(hWnd, WM_WTSSESSION_CHANGE, WTS_REMOTE_CONNECT, 0);

		WTSRegisterSessionNotification(hWnd, NOTIFY_FOR_ALL_SESSIONS);
		break;
	case WM_WTSSESSION_CHANGE:
	{
		if (hGlobal_SkypeAPIWindowHandle)
		{
			BOOL b = TRUE;
			switch (wParam)
			{
			case WTS_REMOTE_CONNECT:
			{
				CHAR szComputerName[MAX_PATH];
				lstrcpyA(szComputerName, "SET PROFILE MOOD_TEXT");
				PWTS_SESSION_INFO ppSessionInfo = 0;
				DWORD pCount;
				if (WTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE, 0, 1, &ppSessionInfo, &pCount))
				{
					for (unsigned int i = 0; i<pCount; i++)
					{
						if (lstrcmp(ppSessionInfo[i].pWinStationName, TEXT("RDP-Tcp#0")) == 0)
						{
							DWORD bytesReturned = 0;
							LPSTR pData = 0;
							if (WTSQuerySessionInformationA(WTS_CURRENT_SERVER_HANDLE, ppSessionInfo[i].SessionId, WTSClientName, &pData, &bytesReturned))
							{
								lstrcatA(szComputerName, " ");
								lstrcatA(szComputerName, pData);
							}
							WTSFreeMemory(pData);
						}
					}
				}
				(b = (BOOL)SendSkypeMessage(hWnd, hGlobal_SkypeAPIWindowHandle, "SET USERSTATUS ONLINE")) &&
					(b = (BOOL)SendSkypeMessage(hWnd, hGlobal_SkypeAPIWindowHandle, szComputerName));
			}
			break;
			case WTS_REMOTE_DISCONNECT:
				(b = (BOOL)SendSkypeMessage(hWnd, hGlobal_SkypeAPIWindowHandle, "SET USERSTATUS AWAY")) &&
					(b = (BOOL)SendSkypeMessage(hWnd, hGlobal_SkypeAPIWindowHandle, "SET PROFILE MOOD_TEXT"));
				break;
			}
			if (!b)//送信がミスった場合
			{
				SendMessage(HWND_BROADCAST, uiGlobal_MsgID_SkypeControlAPIDiscover, (WPARAM)hWnd, 0);
			}
		}
	}
	break;
	case WM_DESTROY:
		if (hGlobal_SkypeAPIWindowHandle)
		{
			SendSkypeMessage(hWnd, hGlobal_SkypeAPIWindowHandle, "SET USERSTATUS AWAY");
			SendSkypeMessage(hWnd, hGlobal_SkypeAPIWindowHandle, "SET PROFILE MOOD_TEXT");
		}
		WTSUnRegisterSessionNotification(hWnd);
		PostQuitMessage(0);
		break;
	default:
		if (uiGlobal_MsgID_SkypeControlAPIAttach&&msg == uiGlobal_MsgID_SkypeControlAPIAttach)
		{
			if (lParam == 0)
			{
				hGlobal_SkypeAPIWindowHandle = (HWND)wParam;
			}
			return 1;
		}
		else
		{
			return DefWindowProc(hWnd, msg, wParam, lParam);
		}
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreInst, LPSTR pCmdLine, int nCmdShow)
{
	MSG msg = { 0 };
	HANDLE hMutex = CreateMutex(NULL, FALSE, szClassName);
	if (GetLastError() != ERROR_ALREADY_EXISTS)
	{
		WNDCLASS wndclass = {
		0,
		WndProc,
		0,
		0,
		hInstance,
		0,
		0,
		0,
		0,
		szClassName
		};
		RegisterClass(&wndclass);
		HWND hWnd = CreateWindowEx(
			WS_EX_TOOLWINDOW,
			szClassName,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			hInstance,
			0
		);
		while (GetMessage(&msg, 0, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		ReleaseMutex(hMutex);
	}
	CloseHandle(hMutex);
	return (int)msg.wParam;
}