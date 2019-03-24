#include <Windows.h>
#include <stdbool.h>
#include <stdio.h>

#pragma comment(lib, "gdi32")

//Vista and later systems only
typedef HANDLE D3DKMT_HANDLE;
typedef LONG D3DDDI_VIDEO_PRESENT_SOURCE_ID;
typedef LONG NTSTATUS;

#define STATUS_SUCCESS 0

typedef struct _D3DKMT_WAITFORVERTICALBLANKEVENT {
  D3DKMT_HANDLE                  hAdapter;
  D3DKMT_HANDLE                  hDevice;
  D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
} D3DKMT_WAITFORVERTICALBLANKEVENT;

typedef struct _D3DKMT_OPENADAPTERFROMHDC {
  HDC                            hDc;
  D3DKMT_HANDLE                  hAdapter;
  LUID                           AdapterLuid;
  D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
} D3DKMT_OPENADAPTERFROMHDC;

typedef struct _D3DKMT_CLOSEADAPTER {
  D3DKMT_HANDLE hAdapter;
} D3DKMT_CLOSEADAPTER;

NTSTATUS WINAPI (*D3DKMTWaitForVerticalBlankEvent)(
  const D3DKMT_WAITFORVERTICALBLANKEVENT *Arg1
);

NTSTATUS WINAPI (*D3DKMTOpenAdapterFromHdc)(
  D3DKMT_OPENADAPTERFROMHDC *Arg1
);

NTSTATUS WINAPI (*D3DKMTCloseAdapter)(
  const D3DKMT_CLOSEADAPTER *Arg1
);

D3DKMT_OPENADAPTERFROMHDC argD3dKmtHdcGlobal;
D3DKMT_WAITFORVERTICALBLANKEVENT argD3dKmtVsyncGlobal;

NTSTATUS nStatusGlobal;

bool InitVsync(HDC hdc) {
	HMODULE hModGdi32 = GetModuleHandle("Gdi32.dll");
	D3DKMTWaitForVerticalBlankEvent = (typeof(D3DKMTWaitForVerticalBlankEvent))GetProcAddress(hModGdi32, "D3DKMTWaitForVerticalBlankEvent");
	D3DKMTOpenAdapterFromHdc = (typeof(D3DKMTOpenAdapterFromHdc))GetProcAddress(hModGdi32, "D3DKMTOpenAdapterFromHdc");
	D3DKMTCloseAdapter = (typeof(D3DKMTCloseAdapter))GetProcAddress(hModGdi32, "D3DKMTCloseAdapter");

	argD3dKmtHdcGlobal.hDc = hdc;
	if ((nStatusGlobal = D3DKMTOpenAdapterFromHdc(&argD3dKmtHdcGlobal)) != STATUS_SUCCESS)
		return false;
	argD3dKmtVsyncGlobal.hAdapter = argD3dKmtHdcGlobal.hAdapter;
	argD3dKmtVsyncGlobal.hDevice = NULL;
	argD3dKmtVsyncGlobal.VidPnSourceId = argD3dKmtHdcGlobal.VidPnSourceId;

	return true;
}

bool WaitForVsync(void) {
	return D3DKMTWaitForVerticalBlankEvent(&argD3dKmtVsyncGlobal) == STATUS_SUCCESS;
}

void CleanupVsync(void) {
	D3DKMT_CLOSEADAPTER d3dkmt_ca;
	d3dkmt_ca.hAdapter = argD3dKmtVsyncGlobal.hAdapter;
	D3DKMTCloseAdapter(&d3dkmt_ca);
}

HDC hdcGlobal;

void WndProc_Close(HWND hwnd) {
	ReleaseDC(hwnd, hdcGlobal);
	hdcGlobal = NULL;
	CleanupVsync();
}

void WndProc_Destroy(HWND hwnd) {
	PostQuitMessage(0);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_CLOSE:
		WndProc_Close(hwnd);
		break;
	case WM_DESTROY:
		WndProc_Destroy(hwnd);
		break;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	WNDCLASS wc = { 0 };
	HWND hwnd;
	MSG msg;

	wc.hInstance = hInstance;
	wc.lpfnWndProc = WndProc;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszClassName = "MyWndClass";
	RegisterClass(&wc);

	hwnd = CreateWindow(
		"MyWndClass",
		"Vsync Test",
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		NULL,
		NULL,
		hInstance,
		NULL
	);

	hdcGlobal = GetDC(hwnd);

	if (!InitVsync(hdcGlobal)) {
		char msg[1000];
		sprintf(msg, "No vsync!\nStatus: 0x%08x\n", nStatusGlobal);
		MessageBox(hwnd, msg, NULL, MB_ICONERROR);
	}

	while (true) {
		static int nCounter = 0;
		char buf[1000];
		POINT ptMouse;

		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT)
				break;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			//Draw
			sprintf(buf, "Frame %d", nCounter++);

			GetCursorPos(&ptMouse);
			ScreenToClient(hwnd, &ptMouse);

			TextOut(hdcGlobal, 0, 0, buf, strlen(buf));
			//Ellipse(hdcGlobal, nCounter, nCounter * 1.1, nCounter * 1.1 + 100, nCounter + 100);
			Ellipse(hdcGlobal, ptMouse.x - 50, ptMouse.y - 50, ptMouse.x + 50, ptMouse.y + 50);
			WaitForVsync();

			if (nCounter > 200) {
				FillRect(hdcGlobal, &(RECT) { 0, 0, 9999, 9999 }, (HBRUSH)GetStockObject(WHITE_BRUSH));
				nCounter = 0;
			}
		}
	}

	return 0;
}