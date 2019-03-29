#include <Windows.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

//Referenced from:
//	https://chromium.googlesource.com/chromium/src.git/+/62.0.3178.1/gpu/ipc/service/gpu_vsync_provider_win.cc

#pragma comment(lib, "gdi32")

//Vista and later systems only
typedef LONG NTSTATUS;
typedef UINT D3DKMT_HANDLE;
typedef UINT D3DDDI_VIDEO_PRESENT_SOURCE_ID;

#define STATUS_SUCCESS 0

typedef struct _D3DKMT_WAITFORVERTICALBLANKEVENT {
	D3DKMT_HANDLE hAdapter;
	D3DKMT_HANDLE hDevice;
	D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
} D3DKMT_WAITFORVERTICALBLANKEVENT;

typedef struct _D3DKMT_OPENADAPTERFROMHDC {
	HDC hDc;
	D3DKMT_HANDLE hAdapter;
	LUID AdapterLuid;
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
	argD3dKmtVsyncGlobal.hDevice = 0;
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

unsigned int GetCurrentScreenRefreshRate(void) {
	DEVMODE dm;
	EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm);
	return (unsigned int)dm.dmDisplayFrequency;
}

HDC hdcGlobal;
bool bClearStatistics = false;

void WndProc_Close(HWND hwnd) {
	ReleaseDC(hwnd, hdcGlobal);
	hdcGlobal = NULL;
	CleanupVsync();
}

void WndProc_Destroy(HWND hwnd) {
	PostQuitMessage(0);
}

void WndProc_RightButtonDown(HWND hwnd, DWORD dwMouseKeyState, int xMouse, int yMouse) {
	bClearStatistics = true;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_CLOSE:
		WndProc_Close(hwnd);
		break;
	case WM_DESTROY:
		WndProc_Destroy(hwnd);
		break;
	case WM_RBUTTONDOWN:
		WndProc_RightButtonDown(hwnd, (DWORD)wParam, MAKEPOINTS(lParam).x, MAKEPOINTS(lParam).y);
		break;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	clock_t clkBegin, clkTemp, clkPast;
	unsigned int nCurScreenRefreshHz;
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

	nCurScreenRefreshHz = GetCurrentScreenRefreshRate();

	clkBegin = clock();

	while (true) {
		static int nFramesCount = 0, nFramesDropped = 0;
		char buf[1000];
		POINT ptMouse;

		if (bClearStatistics) {
			bClearStatistics = false;
			clkBegin = clock();
			nFramesCount = 0;
			nFramesDropped = 0;
		}

		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			if (msg.message == WM_QUIT)
				break;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			//Draw
			static double fx = 0, fy = 0, fWidth = 100, fHeight = 100;
			static double fxStep = 10, fyStep = 10;
			RECT rtClient;

			clkTemp = clock();

			//1) Draw
			GetCursorPos(&ptMouse);
			ScreenToClient(hwnd, &ptMouse);
			GetClientRect(hwnd, &rtClient);

			FillRect(hdcGlobal, &rtClient, (HBRUSH)GetStockObject(WHITE_BRUSH));
			TextOut(hdcGlobal, 0, 0, buf, strlen(buf));
			//Ellipse(hdcGlobal, nCounter, nCounter * 1.1, nCounter * 1.1 + 100, nCounter + 100);
			Ellipse(hdcGlobal, ptMouse.x - 50, ptMouse.y - 50, ptMouse.x + 50, ptMouse.y + 50);
			Rectangle(hdcGlobal, fx, fy, fx + fWidth, fy + fHeight);

			//2) Update
			sprintf(
				buf,
				"Frame %d / %d (%.2f fps)",
				nFramesCount,
				nFramesDropped,
				1000.0 * nFramesCount / (clkTemp - clkBegin)
			);
			{
				if (fx < 0)
					fxStep = fabs(fxStep);
				else if (fx + fWidth >= rtClient.right)
					fxStep = -fabs(fxStep);
				if (fy < 0)
					fyStep = fabs(fyStep);
				else if (fy + fHeight >= rtClient.bottom)
					fyStep = -fabs(fyStep);
				fx += fxStep;
				fy += fyStep;
			}

			WaitForVsync();

			//Dropped frames calculation may be not accurate
			nFramesCount++;
			clkPast = clock() - clkTemp;
			clkPast = clkPast < 16 ? 16 : clkPast;
			nFramesDropped += (int)lround(clkPast / (1000.0 / nCurScreenRefreshHz)) - 1;
			/*
			if (clock() - clkTemp > 30)
				nFramesDropped++;	//Maybe not accurate
			*/
		}
	}

	return 0;
}