#pragma once
#include <d3d11.h>
#include "Core/Public/Log.h"
#include "D3D11Enum.h"
#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

/*DXTraceW(__FILEW__, (DWORD)__LINE__, hr, L#x);\*/
void WINAPI DXTraceW(_In_z_ const WCHAR* strFile, _In_ DWORD dwLine, _In_ HRESULT hr, _In_opt_ const WCHAR* strMsg);

#if defined(DEBUG) | defined(_DEBUG)
#define DX_CHECK(x)\
    do{\
        HRESULT hr = (x);\
        if(FAILED(hr)){\
			DXTraceW(__FILEW__, (DWORD)__LINE__, hr, L#x);\
			ASSERT(0, "");\
        }\
    }while(0)
#else
#define DX_CHECK(x) (x)
#endif

#define DX_RELEASE(x) if(x)(x)->Release(); (x)=nullptr