#include "D3D11Util.h"

void WINAPI DXTraceW(_In_z_ const WCHAR* strFile, _In_ DWORD dwLine, _In_ HRESULT hr, _In_opt_ const WCHAR* strMsg) {
    WCHAR e[256];
    FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, hr, MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), e, 256, nullptr); \
        std::wprintf(L"[DX_ERROR]%ls: %lu 0x%0.8x, %ls", strFile, dwLine, hr, e);
}