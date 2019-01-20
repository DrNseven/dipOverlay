/*
	Direct2D Overlay by Coltonon
	Simple library used to make a Direct2d overlay over any application quickly.
	Simply set up your drawing loop, and initialize the overlay.

	Sample use:

-----------------------------------------------------------------------------------------
	#include "DirectOverlay.h"

	void drawLoop(int width, int height) {  // our loop to render in
		DrawString("The quick brown fox jumped over the lazy dog", 48, 100, 20, 0, 1, 1);
	}

	int main(){
		DirectOverlaySetOption(D2DOV_DRAW_FPS | D2DOV_FONT_IMPACT);	// set the font, and draw the fps
		DirectOverlaySetup(drawLoop);		// initialize our overlay
		getchar();	// The overlay operates in it's own thread, ours will continue as normal
	}
-----------------------------------------------------------------------------------------

	The #defines are settings for the window, pass them to the DirectOverlaySetOption function.
	You may OR them together '|', you must set them before calling DirectOverlaySetup.

*/

#include <Windows.h>
#include <string>
#pragma warning (disable: 4244)

// Link the static library (make sure that file is in the same directory as this file)
//#pragma comment(lib, "D2DOverlay.lib")

// Requires the targetted window to be active and the foreground window to draw.
#define D2DOV_REQUIRE_FOREGROUND	(1 << 0)

// Draws the FPS of the overlay in the top-right corner
#define D2DOV_DRAW_FPS				(1 << 1)

// Attempts to limit the frametimes so you don't render at 500fps
#define D2DOV_VSYNC					(1 << 2)

// Sets the text font to Calibri
#define D2DOV_FONT_CALIBRI			(1 << 3)

// Sets the text font to Arial
#define D2DOV_FONT_ARIAL			(1 << 4)

// Sets the text font to Courier
#define D2DOV_FONT_COURIER			(1 << 5)

// Sets the text font to Gabriola
#define D2DOV_FONT_GABRIOLA			(1 << 6)

// Sets the text font to Impact
#define D2DOV_FONT_IMPACT			(1 << 7)

// The function you call to set up the above options.  Make sure its called before the DirectOverlaySetup function
void DirectOverlaySetOption(DWORD option);

// typedef for the callback function, where you'll do the drawing.
typedef void(*DirectOverlayCallback)(int width, int height);

// Initializes a the overlay window, and the thread to run it.  Input your callback function.
// Uses the first window in the current process to target.  If you're external, use the next function
void DirectOverlaySetup(DirectOverlayCallback callbackFunction);

// Used to specify the window manually, to be used with externals.
void DirectOverlaySetup(DirectOverlayCallback callbackFunction, HWND targetWindow);

// Draws a line from (x1, y1) to (x2, y2), with a specified thickness.
// Specify the color, and optionally an alpha for the line.
void DrawLine(float x1, float y1, float x2, float y2, float thickness, float r, float g, float b, float a = 1);

// Draws a rectangle on the screen.  Width and height are relative to the coordinates of the box.  
// Use the "filled" bool to make it a solid rectangle; ignore the thickness.
// To just draw the border around the rectangle, specify a thickness and pass "filled" as false.
void DrawBox(float x, float y, float width, float height, float thickness, float r, float g, float b, float a, bool filled);

// Draws a circle.  As with the DrawBox, the "filled" bool will make it a solid circle, and thickness is only used when filled=false.
void DrawCircle(float x, float y, float radius, float thickness, float r, float g, float b, float a, bool filled);

// Allows you to draw an elipse.  Same as a circle, except you have two different radii, for width and height.
void DrawEllipse(float x, float y, float width, float height, float thickness, float r, float g, float b, float a, bool filled);

// Draw a string on the screen.  Input is in the form of an std::string.
void DrawString(std::string str, float fontSize, float x, float y, float r, float g, float b, float a = 1);



#include <d2d1.h>
#include <dwrite.h>
#include <dwmapi.h>
#include <fstream>
#include <comdef.h>
#include <iostream>
#include <ctime>
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "dwrite.lib")

ID2D1Factory* factory;
ID2D1HwndRenderTarget* target;
ID2D1SolidColorBrush* solid_brush;
IDWriteFactory* w_factory;
IDWriteTextFormat* w_format;
IDWriteTextLayout* w_layout;
HWND overlayWindow;
HINSTANCE appInstance;
HWND targetWindow;
HWND enumWindow = NULL;
time_t preTime = clock();
time_t showTime = clock();
int fps = 0;

bool o_Foreground = false;
bool o_DrawFPS = false;
bool o_VSync = false;
std::wstring fontname = L"Courier";

DirectOverlayCallback drawLoopCallback = NULL;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void DrawString(std::string str, float fontSize, float x, float y, float r, float g, float b, float a)
{
	RECT re;
	GetClientRect(overlayWindow, &re);
	FLOAT dpix, dpiy;
	dpix = static_cast<float>(re.right - re.left);
	dpiy = static_cast<float>(re.bottom - re.top);
	HRESULT res = w_factory->CreateTextLayout(std::wstring(str.begin(), str.end()).c_str(), str.length() + 1, w_format, dpix, dpiy, &w_layout);
	if (SUCCEEDED(res))
	{
		DWRITE_TEXT_RANGE range = { 0, str.length() };
		w_layout->SetFontSize(fontSize, range);
		solid_brush->SetColor(D2D1::ColorF(r, g, b, a));
		target->DrawTextLayout(D2D1::Point2F(x, y), w_layout, solid_brush);
		w_layout->Release();
		w_layout = NULL;
	}
}

void DrawBox(float x, float y, float width, float height, float thickness, float r, float g, float b, float a, bool filled)
{
	solid_brush->SetColor(D2D1::ColorF(r, g, b, a));
	if (filled)  target->FillRectangle(D2D1::RectF(x, y, x + width, y + height), solid_brush);
	else target->DrawRectangle(D2D1::RectF(x, y, x + width, y +height), solid_brush, thickness);
}

void DrawLine(float x1, float y1, float x2, float y2, float thickness, float r, float g, float b, float a) {
	solid_brush->SetColor(D2D1::ColorF(r, g, b, a));
	target->DrawLine(D2D1::Point2F(x1, y1), D2D1::Point2F(x2, y2), solid_brush, thickness);
}

void DrawCircle(float x, float y, float radius, float thickness, float r, float g, float b, float a, bool filled)
{
	solid_brush->SetColor(D2D1::ColorF(r, g, b, a));
	if (filled) target->FillEllipse(D2D1::Ellipse(D2D1::Point2F(x, y), radius, radius), solid_brush);
	else target->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(x, y), radius, radius), solid_brush, thickness);
}

void DrawEllipse(float x, float y, float width, float height, float thickness, float r, float g, float b, float a, bool filled)
{
	solid_brush->SetColor(D2D1::ColorF(r, g, b, a));
	if (filled) target->FillEllipse(D2D1::Ellipse(D2D1::Point2F(x, y), width, height), solid_brush);
	else target->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(x, y), width, height), solid_brush, thickness); 
}

void d2oSetup(HWND tWindow) {
	targetWindow = tWindow;
	WNDCLASS wc = { };
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = GetModuleHandle(0);
	wc.lpszClassName = L"d2do";
	RegisterClass(&wc);
	overlayWindow = CreateWindowEx(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST, 
		wc.lpszClassName, L"D2D Overlay", WS_POPUP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 
		NULL, NULL, wc.hInstance, NULL);
	
	MARGINS mar = { -1 };
	DwmExtendFrameIntoClientArea(overlayWindow, &mar);
	D2D1CreateFactory(D2D1_FACTORY_TYPE_MULTI_THREADED, &factory);
	factory->CreateHwndRenderTarget(
		D2D1::RenderTargetProperties(D2D1_RENDER_TARGET_TYPE_DEFAULT,
			D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED)),
		D2D1::HwndRenderTargetProperties(overlayWindow, D2D1::SizeU(200, 200), 
			D2D1_PRESENT_OPTIONS_IMMEDIATELY), &target);
	target->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.0f, 0.0f), &solid_brush);
	target->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
	DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&w_factory));
	w_factory->CreateTextFormat(fontname.c_str(), NULL, DWRITE_FONT_WEIGHT_NORMAL, 
		DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 10.0f, L"en-us", &w_format);
}

void mainLoop() {
	MSG message;
	message.message = WM_NULL;
	ShowWindow(overlayWindow, 1);
	UpdateWindow(overlayWindow);
	SetLayeredWindowAttributes(overlayWindow, RGB(0, 0, 0), 255, LWA_ALPHA);
	if (message.message != WM_QUIT)
	{
		if (PeekMessage(&message, overlayWindow, NULL, NULL, PM_REMOVE))
		{
			TranslateMessage(&message);
			DispatchMessage(&message);
		}

		UpdateWindow(overlayWindow);
		WINDOWINFO info;
		ZeroMemory(&info, sizeof(info));
		info.cbSize = sizeof(info);
		GetWindowInfo(targetWindow, &info);
		D2D1_SIZE_U siz;
		siz.height = ((info.rcClient.bottom) - (info.rcClient.top));
		siz.width = ((info.rcClient.right) - (info.rcClient.left));
		if (!IsIconic(overlayWindow)) {
			SetWindowPos(overlayWindow, NULL, info.rcClient.left, info.rcClient.top, siz.width, siz.height, SWP_SHOWWINDOW);
			target->Resize(&siz);
		}
		target->BeginDraw();
		target->Clear(D2D1::ColorF(0, 0, 0, 0));
		if (drawLoopCallback != NULL) {
			if (o_Foreground) {
				if (GetForegroundWindow() == targetWindow) 
					goto toDraw;
				else goto noDraw;
			}

			toDraw: 
			time_t postTime = clock();
			time_t frameTime = postTime - preTime;
			preTime = postTime;

			if (o_DrawFPS) {
				if (postTime - showTime > 100) {
					fps = 1000 / (float)frameTime;
					showTime = postTime;
				}
				DrawString(std::to_string(fps), 20, siz.width - 50, 0, 0, 1, 0);
			}

			if (o_VSync) {
				int pausetime = 17 - frameTime;
				if (pausetime > 0 && pausetime < 30) {
					Sleep(pausetime);
				}
			}
			
			drawLoopCallback(siz.width, siz.height);
		}
		noDraw:
		target->EndDraw();
		Sleep(1);
	}
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uiMessage, WPARAM wParam, LPARAM lParam)
{
	switch (uiMessage)
	{
	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, uiMessage, wParam, lParam);
	}
	return 0;
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
	DWORD lpdwProcessId;
	GetWindowThreadProcessId(hwnd, &lpdwProcessId);
	if (lpdwProcessId == GetCurrentProcessId())
	{
		enumWindow = hwnd;
		return FALSE;
	}
	return TRUE;
}

DWORD WINAPI OverlayThread(LPVOID lpParam)
{
	if (lpParam == NULL) {
		EnumWindows(EnumWindowsProc, NULL);
	}
	else {
		enumWindow = (HWND)lpParam;
	}
	d2oSetup(enumWindow);
	for (;;) {
		mainLoop();
	}
}

void DirectOverlaySetup(DirectOverlayCallback callback) {
	drawLoopCallback = callback;
	CreateThread(0, 0, OverlayThread, NULL, 0, NULL);
}

void DirectOverlaySetup(DirectOverlayCallback callback, HWND _targetWindow) {
	drawLoopCallback = callback;
	CreateThread(0, 0, OverlayThread, _targetWindow, 0, NULL);
}

void DirectOverlaySetOption(DWORD option) {
	if (option & D2DOV_REQUIRE_FOREGROUND) o_Foreground = true;
	if (option & D2DOV_DRAW_FPS) o_DrawFPS = true;
	if (option & D2DOV_VSYNC) o_VSync = true;
	if (option & D2DOV_FONT_ARIAL) fontname = L"arial";
	if (option & D2DOV_FONT_COURIER) fontname = L"Courier";
	if (option & D2DOV_FONT_CALIBRI) fontname = L"Calibri";
	if (option & D2DOV_FONT_GABRIOLA) fontname = L"Gabriola";
	if (option & D2DOV_FONT_IMPACT) fontname = L"Impact";
}
