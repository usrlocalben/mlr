
//#pragma comment(linker,"/SUBSYSTEM:CONSOLE")
//#pragma comment(linker,"/SUBSYSTEM:WINDOWS")

#include "stdafx.h"

#include <iostream>
#include <boost/format.hpp>

#include <Windows.h>

#include "main.h"

using namespace std;
using boost::format;


int hackmode_enable = 0;
int hackmode_height = 0;
int hackmode_width = 0;

void bind_to_cpu0()
{
	const HANDLE self = GetCurrentThread();
	const DWORD_PTR pmask = SetThreadAffinityMask(self, 1);

	if (0) {
		const DWORD_PTR pmask2 = SetThreadAffinityMask(self, 1);
		cout << "original pmask was " << format("%x") % pmask << ", new pmask is " << format("%x") % pmask2 << endl;
	}
}




int _tmain(int argc, _TCHAR* argv[])
{
	//bind_to_cpu0();
//	sse_speedup();


	if (!SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS)) {
		DWORD dwError = GetLastError();
		cout << "failed to set process mode." << endl;
	}

	Application app;
	//hackmode_enable = true;
	//hackmode_width = 1280;
	//hackmode_height = 720;
	app.run(false);
	return 0;
}


#include "resource.h"

const TCHAR *igpu[] = {
	TEXT("idle"),
	TEXT("away"),
	TEXT("hiatus"),
	TEXT("resting"),
	TEXT("vacation"),
	TEXT("stone cold chillin'")
};

bool run_demo = false;
bool run_fullscreen = false;
bool run_tdfmode = false;
int  run_screenmode = -1;


INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
		case WM_INITDIALOG: {
			HWND combogpu = GetDlgItem(hDlg, IDC_GPU);
			for (int i = 0; i < sizeof(igpu) / sizeof(TCHAR*); i++) {
				SendMessage(combogpu, CB_ADDSTRING, 0, (LPARAM)igpu[i]);
			}
			SendMessage(combogpu, CB_SETCURSEL, 0, 0);
		}
		break;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDCANCEL:
					SendMessage(hDlg, WM_CLOSE, 0, 0);
					return TRUE;
				case IDOK:
					run_demo = true;
					SendMessage(hDlg, WM_CLOSE, 0, 0);
					return TRUE;
			}
			break;

		case WM_CLOSE:
			run_fullscreen = SendMessage(GetDlgItem(hDlg, IDC_FULLSCREEN), BM_GETCHECK, 0, 0) > 0 ? true : false; // silence bool performance warning
			run_tdfmode = SendMessage(GetDlgItem(hDlg, IDC_TDFMODE), BM_GETCHECK, 0, 0) > 0 ? true : false;
			//run_screenmode = SendMessage( GetDlgItem(hDlg,IDC_RES), CB_GETCURSEL, 0, 0 );
			DestroyWindow(hDlg);
			return TRUE;

		case WM_DESTROY:
			PostQuitMessage(0);
			return TRUE;
	}
	return FALSE;
}


int WINAPI _tWinMain(HINSTANCE hInst, HINSTANCE h0, LPTSTR lpCmdLine, int nCmdShow)
{
	HWND hDlg;
	MSG msg;
	BOOL ret;

	hDlg = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_DIALOG1), 0, DialogProc, 0);
	ShowWindow(hDlg, nCmdShow);

	while ((ret = GetMessage(&msg, 0, 0, 0)) != 0) {
		if (ret == -1) return -1;

		if (!IsDialogMessage(hDlg, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	if (run_demo) {
//		sse_speedup();
		Application app;
		if (run_tdfmode) {
			hackmode_enable = 1;
			hackmode_width = 1280;
			hackmode_height = 720;
		} else {
			hackmode_enable = 0;
		}
		app.fullscreen = run_fullscreen;
		app.run(true);
	}

	return 0;
}
