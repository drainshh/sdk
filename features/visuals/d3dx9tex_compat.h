#pragma once

// Minimal compatibility header providing the few D3DX9 declarations used by the project
// This avoids requiring the legacy "d3dx9tex.h" header to be installed with the Windows SDK

#include <Windows.h>
#include <d3d9.h>

#ifndef D3DX_DEFAULT
#define D3DX_DEFAULT ((UINT)-1)
#endif

#ifndef D3DX_FILTER_NONE
#define D3DX_FILTER_NONE 0
#endif
#ifndef D3DX_FILTER_POINT
#define D3DX_FILTER_POINT 1
#endif
#ifndef D3DX_FILTER_LINEAR
#define D3DX_FILTER_LINEAR 2
#endif

// Minimal D3DXIMAGE_INFO struct (the project only passes nullptr for these, but declare for completeness)
typedef struct _D3DXIMAGE_INFO {
    UINT Width;
    UINT Height;
    UINT Depth;
    UINT MipLevels;
    D3DFORMAT Format;
    D3DRESOURCETYPE ResourceType;
    DWORD ImageFileFormat;
} D3DXIMAGE_INFO;

#ifdef __cplusplus
extern "C" {
#endif

// Declarations for the D3DX texture creation functions used in the project.
// The real implementations are provided by "d3dx9.lib"/runtime DLL. These prototypes
// allow the code to compile when the legacy headers are unavailable.

HRESULT WINAPI D3DXCreateTextureFromFileExW(
    IDirect3DDevice9* pDevice,
    LPCWSTR pSrcFile,
    UINT Width,
    UINT Height,
    UINT MipLevels,
    DWORD Usage,
    D3DFORMAT Format,
    D3DPOOL Pool,
    DWORD Filter,
    DWORD MipFilter,
    D3DCOLOR ColorKey,
    D3DXIMAGE_INFO* pSrcInfo,
    PALETTEENTRY* pPalette,
    IDirect3DTexture9** ppTexture
);

HRESULT WINAPI D3DXCreateTextureFromFileInMemoryEx(
    IDirect3DDevice9* pDevice,
    const void* pSrcData,
    SIZE_T SrcDataSize,
    UINT Width,
    UINT Height,
    UINT MipLevels,
    DWORD Usage,
    D3DFORMAT Format,
    D3DPOOL Pool,
    DWORD Filter,
    DWORD MipFilter,
    D3DCOLOR ColorKey,
    D3DXIMAGE_INFO* pSrcInfo,
    PALETTEENTRY* pPalette,
    IDirect3DTexture9** ppTexture
);

#ifdef __cplusplus
}
#endif
