#define DIRECTINPUT_VERSION 0x0800
HWND hWnd = GetForegroundWindow();
//HWND hWnd = FindWindow(NULL, "game window name");

#include <dinput.h>
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dinput8.lib")

LPDIRECTINPUT8 din;				// pointer to the DirectInput interface
LPDIRECTINPUTDEVICE8 dmouse;	// pointer to the mouse device

LPDIRECTINPUTDEVICE8 DIKeyboard;
LPDIRECTINPUTDEVICE8 DIMouse;
LPDIRECTINPUT8 DirectInput;

HRESULT hr;

bool rmouse_down = false;

bool InitInput()
{
	// create a direct input interface
	hr = DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&DirectInput, NULL);

	//create device
	DirectInput->CreateDevice(GUID_SysKeyboard, &DIKeyboard, NULL);
	DirectInput->CreateDevice(GUID_SysMouse, &DIMouse, NULL);

	DIKeyboard->SetDataFormat(&c_dfDIKeyboard);
	DIKeyboard->SetCooperativeLevel(hWnd, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE);

	//set format
	DIMouse->SetDataFormat(&c_dfDIMouse);

	//set control you will have over the device
	DIMouse->SetCooperativeLevel(hWnd, DISCL_BACKGROUND | DISCL_NONEXCLUSIVE);
	return true;
}

BYTE keyboardState[256];
bool IsKeyPressed(unsigned char DI_keycode)
{
	return keyboardState[DI_keycode] & 0x80;
}

DIMOUSESTATE mouseCurrState; // pointer to the mouse information
void CheckInput()
{
	DIKeyboard->Acquire();

	DIMouse->Acquire();

	DIKeyboard->GetDeviceState(sizeof(keyboardState), (LPVOID)&keyboardState);
	DIMouse->GetDeviceState(sizeof(DIMOUSESTATE), &mouseCurrState);

	// check for button clicks
	if (mouseCurrState.rgbButtons[1] & 0x80) //right mouse button
		rmouse_down = true;
	else rmouse_down = false;
}

void ReleaseInput()
{
	DIKeyboard->Unacquire();
	DIMouse->Unacquire();
	DirectInput->Release();
}
