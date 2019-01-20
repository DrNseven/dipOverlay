#include <Windows.h>
#include <d3d9.h>
#include <vector>
#include <sstream>
#include <fstream>
#include <iostream>
#include <process.h>
#pragma comment(lib, "winmm.lib") //time

//DX Includes
#include <DirectXMath.h>
using namespace DirectX;

//dx sdk, if files are in ..\DXSDK dir
//#include "DXSDK\d3dx9.h"
//#if defined _M_X64
//#pragma comment(lib, "DXSDK/x64/d3dx9.lib") 
//#elif defined _M_IX86
//#pragma comment(lib, "DXSDK/x86/d3dx9.lib")
//#endif

#pragma warning (disable: 4244)
using namespace std;

//stride
IDirect3DVertexBuffer9* pStreamData = NULL;
UINT xOffset, Stride;

//numelements
IDirect3DVertexDeclaration9* pDecl;
D3DVERTEXELEMENT9 decl[MAXD3DDECLLENGTH];
UINT numElements;

//vsize
IDirect3DVertexShader9* vShader;
UINT vSize;
D3DVERTEXBUFFER_DESC vDesc;

//psize
IDirect3DPixelShader9* pShader;
UINT pSize;

//gettexture
DWORD pStage;
IDirect3DBaseTexture9* pTexture;
IDirect3DTexture9 *pCurrentTex;
D3DSURFACE_DESC sDesc;

//idesc
IDirect3DIndexBuffer9* pIndexData;
D3DINDEXBUFFER_DESC iDesc;

//viewport
D3DVIEWPORT9 Viewport;
float ScreenCX;
float ScreenCY;

//timer
DWORD astime = timeGetTime();

//log & misc
int countnum = -1;
bool InitOnce = true;
bool InitOnce2 = true;

//=========================================================================================================================//

DWORD Daimkey = VK_RBUTTON;		//aimkey
int aimsens = 10;				//aim sensitivity, makes aim smoother
int aimfov = 3;					//aim field of view in % 
int aimheight = 3;				//aim height value, high value aims higher

double DistX;
double DistY;

//=========================================================================================================================//

char dlldir[320];
char *GetDirectoryFile(char *filename)
{
	static char path[320];
	strcpy_s(path, dlldir);
	strcat_s(path, filename);
	return path;
}

void Log(const char *fmt, ...)
{
	if (!fmt)	return;

	char		text[4096];
	va_list		ap;
	va_start(ap, fmt);
	vsprintf_s(text, fmt, ap);
	va_end(ap);

	ofstream logfile(GetDirectoryFile((PCHAR)"log.txt"), ios::app);
	if (logfile.is_open() && text)	logfile << text << endl;
	logfile.close();
}

/*
VOID doDisassembleShader(LPDIRECT3DDEVICE9 pDevice, char* FileName)
{
	std::ofstream oLogFile(FileName, std::ios::trunc);
	if (!oLogFile.is_open())
		return;
	IDirect3DVertexShader9* pShader;
	pDevice->GetVertexShader(&pShader);
	UINT pSizeOfData;
	pShader->GetFunction(NULL, &pSizeOfData);
	BYTE* pData = new BYTE[pSizeOfData];
	pShader->GetFunction(pData, &pSizeOfData);
	LPD3DXBUFFER bOut;
	D3DXDisassembleShader(reinterpret_cast<DWORD*>(pData), NULL, NULL, &bOut);
	oLogFile << static_cast<char*>(bOut->GetBufferPointer()) << std::endl;
	oLogFile.close();
	delete[] pData;
	pShader->Release();
}
*/

//=========================================================================================================================//

//returnaddress method for finding models
//#pragma intrinsic(_ReturnAddress) //Less indexes
//#pragma intrinsic(_AddressOfReturnAddress) //Has more indexes

int									g_Index = -1;
std::vector<void*>					g_Vector;
void*								g_SelectedAddress = NULL;

bool IsAddressPresent(void* Address)
{
	for (auto it = g_Vector.begin(); it != g_Vector.end(); ++it)
	{
		if (*it == Address)
			return true;
	}
	return false;
}

//=========================================================================================================================//

//calc distance
float GetDistance(float Xx, float Yy, float xX, float yY)
{
	return sqrt((yY - Yy) * (yY - Yy) + (xX - Xx) * (xX - Xx));
}

struct ModelInfo_t
{
	float pOutX, pOutY, RealDistance;
	float CrosshairDistance;
};
std::vector<ModelInfo_t>ModelInfo;

// Registers:
//
//   Name         Reg   Size
//   ------------ ----- ----
//   World        c0       4
//   View         c4       4
//   Projection   c8       4
//   CameraPos    c12      1
//   CameraDir    c32      1

//w2s
void AddModels(LPDIRECT3DDEVICE9 Device)
{
	//gettransform method dxmath
	DirectX::XMVECTOR Target = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	DirectX::XMMATRIX projection, view, world;

	Device->GetTransform(D3DTS_VIEW, reinterpret_cast<D3DMATRIX*>(&view));
	Device->GetTransform(D3DTS_PROJECTION, reinterpret_cast<D3DMATRIX*>(&projection));
	Device->GetTransform(D3DTS_WORLD, reinterpret_cast<D3DMATRIX*>(&world));

	DirectX::XMVECTOR xScreen = DirectX::XMVector3Project(Target, Viewport.X, Viewport.Y, Viewport.Width, Viewport.Height, Viewport.MinZ, Viewport.MaxZ, projection, view, world);

	float xx, yy;
	if (xScreen.m128_f32[2] <= 1)
		if ((xScreen.m128_f32[0] < (FLOAT)Viewport.X || xScreen.m128_f32[0] >(FLOAT)(Viewport.X + Viewport.Width)) || (xScreen.m128_f32[1] < (FLOAT)Viewport.Y || xScreen.m128_f32[1] >(FLOAT)(Viewport.Y + Viewport.Height)))
		{
			xx = -1;
			yy = -1;
		}
		else
		{
			xx = xScreen.m128_f32[0];
			yy = xScreen.m128_f32[1];
		}

	ModelInfo_t pModelInfo = { static_cast<float>(xx), static_cast<float>(yy), static_cast<float>(xScreen.m128_f32[2]) };
	ModelInfo.push_back(pModelInfo);

	/*
	//gettransform method d3dx
	D3DXVECTOR3 Target;
	D3DXMATRIX projection, view, world;

	Device->GetTransform(D3DTS_VIEW, &view);
	Device->GetTransform(D3DTS_PROJECTION, &projection);
	Device->GetTransform(D3DTS_WORLD, &world);

	D3DXVECTOR3 xScreen;
	D3DXVec3Project(&xScreen, &Target, &Viewport, &projection, &view, &world);

	float xx, yy;
	if (xScreen.z <= 1)
	if ((xScreen.x < (FLOAT)Viewport.X || xScreen.x >(FLOAT)(Viewport.X + Viewport.Width)) || (xScreen.y < (FLOAT)Viewport.Y || xScreen.y >(FLOAT)(Viewport.Y + Viewport.Height)))
	{
		xx = -1;
		yy = -1;
	}
	else
	{
		xx = xScreen.x;
		yy = xScreen.y;
	}

	ModelInfo_t pModelInfo = { static_cast<float>(xx), static_cast<float>(yy), static_cast<float>(xScreen.z) };
	ModelInfo.push_back(pModelInfo);
	*/

	/*
	//getvscf method d3dx
	D3DXMATRIX pProjection, pView, pWorld;
	D3DXVECTOR3 vOut(0, 0, 0), vIn(0, 0, 0);

	Device->GetVertexShaderConstantF(8, pProjection, 4);
	Device->GetVertexShaderConstantF(4, pView, 4);
	Device->GetVertexShaderConstantF(0, pWorld, 4);

	D3DXVec3Project(&vOut, &vIn, &Viewport, &pProjection, &pView, &pWorld);

	float xx, yy;
	if (vOut.z < 1.0f && vOut.x > 1 && pProjection._44 < 1.0f)
	{
		xx = vOut.x;
		yy = vOut.y;
	}
	else
	{
		xx = -1.0f;
		yy = -1.0f;
	}

	ModelInfo_t pModelInfo = { static_cast<float>(xx), static_cast<float>(yy), static_cast<float>(vOut.z) };
	ModelInfo.push_back(pModelInfo);
	*/

	/*
	//getvscf method dxmath
	DirectX::XMVECTOR Pos = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	//DirectX::XMFLOAT4 pOut2d;

	DirectX::XMMATRIX pProjection;
	DirectX::XMMATRIX pView;
	DirectX::XMMATRIX pWorld;

	Device->GetVertexShaderConstantF(8, (float*)&pProjection, 4);
	Device->GetVertexShaderConstantF(4, (float*)&pView, 4);
	Device->GetVertexShaderConstantF(0, (float*)&pWorld, 4);

	DirectX::XMVECTOR pOut = DirectX::XMVector3Project(Pos, Viewport.X, Viewport.Y, Viewport.Width, Viewport.Height, Viewport.MinZ, Viewport.MaxZ, pProjection, pView, pWorld);
	//DirectX::XMStoreFloat4(&pOut2d, pOut);

	ModelInfo_t pModelInfo = { static_cast<float>(pOut.m128_f32[0]), static_cast<float>(pOut.m128_f32[1]), static_cast<float>(pOut.m128_f32[2]) };
	ModelInfo.push_back(pModelInfo);
	*/
}

//=========================================================================================================================//

void DrawPoint(IDirect3DDevice9* Device, int baseX, int baseY, int baseW, int baseH, D3DCOLOR Cor)
{
	D3DRECT BarRect = { baseX, baseY, baseX + baseW, baseY + baseH };
	Device->Clear(1, &BarRect, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, Cor, 0, 0);
}

bool Match(const BYTE* pData, const BYTE* bMask, const char* szMask)
{
	for(;*szMask;++szMask,++pData,++bMask)
	if(*szMask=='x' && *pData!=*bMask ) 
		return false;
	return (*szMask) == NULL;
}

DWORD FindPattern(DWORD dwAddress,DWORD dwLen,BYTE *bMask,char * szMask)
{
	for(DWORD i=0; i<dwLen; i++)
		if(Match((BYTE*)(dwAddress + i), bMask, szMask))
			return (DWORD)(dwAddress+i);
	return 0;
}

void MakeJMP(BYTE *pAddress, DWORD dwJumpTo, DWORD dwLen)
{
	DWORD dwOldProtect, dwBkup, dwRelAddr;
	VirtualProtect(pAddress, dwLen, PAGE_EXECUTE_READWRITE, &dwOldProtect);
	dwRelAddr = (DWORD) (dwJumpTo - (DWORD) pAddress) - 5;
	*pAddress = 0xE9;
	*((DWORD *)(pAddress + 0x1)) = dwRelAddr;
	for(DWORD x = 0x5; x < dwLen; x++) *(pAddress + x) = 0x90;
	VirtualProtect(pAddress, dwLen, dwOldProtect, &dwBkup);
	return;
}

BOOL CheckWin7()
{
	DWORD version = GetVersion();
	DWORD major = (DWORD)(LOBYTE(LOWORD(version)));
	DWORD minor = (DWORD)(HIBYTE(LOWORD(version)));
	return ((major == 6) && (minor == 1));
}

BOOL CheckWin10() 
{
	DWORD version = GetVersion();
	DWORD major = (DWORD)(LOBYTE(LOWORD(version)));
	DWORD minor = (DWORD)(HIBYTE(LOWORD(version)));
	return ((major == 10) && (minor == 0));
}

template <int XORSTART, int BUFLEN, int XREFKILLER>

class XorStr
{
private:
	XorStr();
public:
	char s[BUFLEN];

	XorStr(const char * xs);

	~XorStr()
	{
		for (int i = 0; i < BUFLEN; i++) s[i] = 0;
	}
};

template <int XORSTART, int BUFLEN, int XREFKILLER>
XorStr<XORSTART, BUFLEN, XREFKILLER>::XorStr(const char * xs)
{
	int xvalue = XORSTART;
	int i = 0;

	for (; i < (BUFLEN - 1); i++)
	{
		s[i] = xs[i - XREFKILLER] ^ xvalue;
		xvalue += 1;
		xvalue %= 256;
	}

	s[BUFLEN - 1] = 0;
}

#define ed3d9		/*d3d9.dll*/XorStr<0xB9,9,0x64C42EE0>("\xDD\x89\xDF\x85\x93\xDA\xD3\xAC"+0x64C42EE0).s
