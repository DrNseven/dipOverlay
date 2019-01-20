//dipOverlay

#include "direct2d.h"
#include "main.h"
#include "directinput.h"

DWORD retMyDIP;

void DIPfunc (LPDIRECT3DDEVICE9 pDevice)
{
	//if ( (InitOnce)||(IsKeyPressed2(DIK_ESCAPE)))
	//{
		//InitOnce = false;

		//get viewport
		pDevice->GetViewport(&Viewport);
		ScreenCX = (float)Viewport.Width / 2.0f;
		ScreenCY = (float)Viewport.Height / 2.0f;
	//}

	//get stride
	if (pDevice->GetStreamSource(0, &pStreamData, &xOffset, &Stride) == D3D_OK)
		//if (pStreamData && Stride == 32) { pStreamData->GetDesc(&vDesc); }
		if (pStreamData != NULL){ pStreamData->Release(); pStreamData = NULL; }

	//grab numelements (remove if not used)
	pDevice->GetVertexDeclaration(&pDecl);
	if (pDecl != NULL)
		pDecl->GetDeclaration(decl, &numElements);
	if (pDecl != NULL) { pDecl->Release(); pDecl = NULL; }

	//get vSize (remove if not used)
	if (SUCCEEDED(pDevice->GetVertexShader(&vShader)))
		if (vShader != NULL)
			if (SUCCEEDED(vShader->GetFunction(NULL, &vSize)))
				if (vShader != NULL) { vShader->Release(); vShader = NULL; }

	//get pSize (remove if not used)
	if (SUCCEEDED(pDevice->GetPixelShader(&pShader)))
		if (pShader != NULL)
			if (SUCCEEDED(pShader->GetFunction(NULL, &pSize)))
				if (pShader != NULL) { pShader->Release(); pShader = NULL; }

	//get iDesc.Size (remove if not used)
	if (SUCCEEDED(pDevice->GetIndices(&pIndexData)))
		if (pIndexData != NULL)
			if (SUCCEEDED(pIndexData->GetDesc(&iDesc)))
				if (pIndexData != NULL) { pIndexData->Release(); pIndexData = NULL; }
	
	//gettexture (remove if not used)
	//if (Stride == )
	//{
		//pDevice->GetTexture(0, &pTexture);
		//pCurrentTex = static_cast<IDirect3DTexture9*>(pTexture);

		//if (pCurrentTex)
		//{
			//pCurrentTex->GetLevelDesc(0, &sDesc);
		//}
	//}

	//dump shader
	//if (Stride == ) //can crash if wrong value
		//if(IsKeyPressed(DIK_F10))
		//doDisassembleShader(pDevice, "ws.txt");


	//wallhack
	pDevice->SetRenderState(D3DRS_DEPTHBIAS, 0);
	//pDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
	if (countnum == Stride)
	//if (Stride == 40||Stride == 44)
	{
		//pDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
		float bias = 1000.0f;
		float bias_float = static_cast<float>(-bias);
		bias_float /= 10000.0f;
		pDevice->SetRenderState(D3DRS_DEPTHBIAS, *(DWORD*)&bias_float);
	}

	//worldtoscreen
	if (countnum == Stride)
	//if (Stride == 40||Stride == 44)
		AddModels(pDevice);

	
	//log models 
	if (countnum == Stride) //numElements //vSize/100 //iDesc.Size
		if (IsKeyPressed(DIK_I)) //i key
			Log("Stride == %d && vSize == %d && pSize == %d && decl->Type == %d && numElements == %d && vDesc.Size == %d && iDesc.Size == %d && sDesc.Height == %d", 
				Stride, vSize, pSize, decl->Type, numElements, vDesc.Size, iDesc.Size, sDesc.Height);

	if (countnum == Stride) //numElements //vSize/100 //iDesc.Size
	{
		pDevice->SetTexture(0, NULL);
		pDevice->SetTexture(1, NULL);
	}
	
}

__declspec(naked) HRESULT WINAPI MyDIP()
{
	static LPDIRECT3DDEVICE9 pDevice;
	
	__asm
	{
		MOV EDI,EDI
		PUSH EBP
		MOV EBP,ESP

		MOV EAX,DWORD PTR SS:[EBP + 0x8]
		MOV pDevice,EAX
	}
	
	
	DIPfunc(pDevice);
	
	__asm
	{
		JMP retMyDIP
	}
	
}

void Hook(void*)
{
	DWORD *vtbl;

	// wait for d3d9.dll
	DWORD hD3D = 0;
	do {
		hD3D = (DWORD)GetModuleHandleA(ed3d9);
		Sleep(10);
	} while (!hD3D);
	DWORD address = FindPattern(hD3D, 0x128000, (PBYTE)"\xC7\x06\x00\x00\x00\x00\x89\x86\x00\x00\x00\x00\x89\x86", (PCHAR)"xx????xx????xx");
	

	if(address)
	{
		memcpy(&vtbl,(void*)(address + 2),4);

		//xp and vista vtbl[149]

		if(CheckWin7())
		{
			retMyDIP = vtbl[147] + 0x5;
			MakeJMP((PBYTE)vtbl[147], (DWORD)MyDIP, 0x5);
		}
		else
		{
			//Log("win 8/10");
			retMyDIP = vtbl[148] + 0x5;
			MakeJMP((PBYTE)vtbl[148],(DWORD)MyDIP,0x5);
		}
	}
}


//=========================================================================================================================//

void drawLoop(int width, int height)
{
	if (InitOnce2)
	{
		InitOnce2 = false;

		InitInput(); //init dinput
	}

	CheckInput(); //dinput


	//do esp
	if (ModelInfo.size() != NULL)
	{
		for (unsigned int i = 0; i < ModelInfo.size(); i++)
		{
			if (ModelInfo[i].pOutX > 1.0f && ModelInfo[i].pOutY > 1.0f)
				DrawString("o", 12, ModelInfo[i].pOutX, ModelInfo[i].pOutY, 0, 1, 1);
		}
	}

	//do aim
	if (ModelInfo.size() != NULL)
	{
		UINT BestTarget = -1;
		DOUBLE fClosestPos = 99999;

		for (unsigned int i = 0; i < ModelInfo.size(); i++)
		{
			//aimfov
			float radiusx = (aimfov*5.0f) * (ScreenCX / 100.0f);
			float radiusy = (aimfov*5.0f) * (ScreenCY / 100.0f);

			if (aimfov == 0)
			{
				radiusx = 5.0f * (ScreenCX / 100.0f);
				radiusy = 5.0f * (ScreenCY / 100.0f);
			}

			//get crosshairdistance
			ModelInfo[i].CrosshairDistance = GetDistance(ModelInfo[i].pOutX, ModelInfo[i].pOutY, ScreenCX, ScreenCY);

			//if in fov
			if (ModelInfo[i].pOutX >= ScreenCX - radiusx && ModelInfo[i].pOutX <= ScreenCX + radiusx && ModelInfo[i].pOutY >= ScreenCY - radiusy && ModelInfo[i].pOutY <= ScreenCY + radiusy)

				//get closest/nearest target to crosshair
				if (ModelInfo[i].CrosshairDistance < fClosestPos)
				{
					fClosestPos = ModelInfo[i].CrosshairDistance;
					BestTarget = i;
				}
		}

		//if nearest target to crosshair
		if (BestTarget != -1) //&& ModelInfo[BestTarget].RealDistance > 4.0f)//do not aim at self
		{
			DistX = ModelInfo[BestTarget].pOutX - ScreenCX;
			DistY = ModelInfo[BestTarget].pOutY - ScreenCY;

			DistX /= (0.5f + (float)aimsens*0.5f);
			DistY /= (0.5f + (float)aimsens*0.5f);

			//aim
			if (rmouse_down) //right mouse
			mouse_event(MOUSEEVENTF_MOVE, (float)DistX, (float)DistY, 0, NULL);
		}
	}
	
	
	//clear less flicker bs
	if (timeGetTime() - astime >= 50) //40-50
	{
		ModelInfo.clear(); //clear
		astime = timeGetTime();
	}


	//logger
	if (IsKeyPressed(DIK_O)) //o key = -
	{
		countnum--;
		Sleep(100);
	}
	if (IsKeyPressed(DIK_P))//p key = +
	{
		countnum++;
		Sleep(100);
	}

	std::stringstream buffer;
	buffer << countnum;
	DrawString(buffer.str().c_str(), 12, 28, 100, 20, 0, 1, 1); //draw variable
	DrawString("dipOverlay", 18, 100, 20, 0, 1, 1);
	//DrawLine(0, 0, 100, 100, 5, 1, 1, 0, .8);
	//DrawBox(100, 100, 100, 100, 5, 0, 1, 0, 1, 0);
	//DrawCircle(dx, dy, 20, 1, 1, 0, 0, .25, 1);
	//DrawEllipse(500, 100, 50, 20, 5, 1, 0, 0, 1, 0);
}

//=========================================================================================================================//

void MainThread(LPVOID lpParam)
{
	//Sleep(1000);

	HMODULE dDll = NULL;
	while (!dDll)
	{
		dDll = GetModuleHandleA("d3d9.dll");
		Sleep(1000);
	}
	CloseHandle(dDll);

	//DirectOverlaySetOption(D2DOV_VSYNC | D2DOV_DRAW_FPS | D2DOV_FONT_ARIAL); //vsync
	DirectOverlaySetOption(D2DOV_DRAW_FPS | D2DOV_FONT_ARIAL);
	DirectOverlaySetup(drawLoop);
	for (;;) { Sleep(100); }
}


//=========================================================================================================================//

extern "C" 
{
    BOOL WINAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
    {
        switch (fdwReason)
        {
		case DLL_PROCESS_DETACH:
			ReleaseInput();
			break;

        case DLL_PROCESS_ATTACH:
			DisableThreadLibraryCalls(hinstDLL);
			//Hook();
			//CreateThread(0,0,(LPTHREAD_START_ROUTINE)Hook,0,0,0);
			HANDLE hThread = (HANDLE)_beginthread(Hook, 0, NULL);
			//CreateThread(0, 0, MainThread, hinstDLL, 0, NULL);
			HANDLE hThread2 = (HANDLE)_beginthread(MainThread, 0, NULL);
            break;
		}
        return true;
    }
}

//XIGNCODE Detects LoadLibrary injection, CreateThread, GetAsyncKeyState, CreateFont, LdrLoadDll, LoadLibraryA, LoadLibraryW, LoadLibraryExA, LoadLibraryExW, GetModuleFileName

//gameguard hooks/blocks
/*
Ring3

advapi32.dll
CreateProcessWithLogonW

gdi32.dll
GetPixel

kernel32.dll
CreateProcessInternalW
DebugActiveProcess
DeviceIoControl
GetProcAddress
LoadLibraryExW
MapViewOfFile
MapViewOfFileEx
MoveFileW
OpenProcess
ReadProcessMemory
VirtualProtect
VirtualProtectEx
WriteProcessMemory

ntdll.dll
NtLoadDriver
NtOpenProcess
NtProtectVirtualMemory
NtQuerySystemInformatio
NtReadVirtualMemory
NtSuspendProcess
NtSuspendThread
NtTerminateProcess
NtTerminateThread
NtWriteVirtualMemory
RtlGetNativeSystemInfor
ZwLoadDriver
ZwOpenProcess
ZwProtectVirtualMemory
ZwQuerySystemInformatio
ZwReadVirtualMemory
ZwSuspendProcess
ZwSuspendThread
ZwTerminateProcess
ZwTerminateThread
ZwWriteVirtualMemory

user32.dll
GetWindowThreadProcessI
PostMessageA
PostMessageW
SendInput
SendMessageA
SendMessageW
SetCursorPos
SetWindowsHookExA
SetWindowsHookExW
keybd_event
mouse_event

Ring0

NtConnectPort
ZwConnectPort
NtOpenProcess
ZwOpenProcess
NtProtectVirtualMemory
ZwProtectVirtualMemory
NtReadVirtualMemory
ZwReadVirtualMemory
NtWriteVirtualMemory
ZwWriteVirtualMemory
SendInput
*/