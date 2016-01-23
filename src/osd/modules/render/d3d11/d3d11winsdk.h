// license:BSD-3-Clause
// copyright-holders: Brad Hughes
//============================================================
//
//  d3d11winsdk.h - Windows API Direct3D 11 headers
//
//============================================================

#ifndef __WIN_D3D11WINSDK__
#define __WIN_D3D11WINSDK__

// standard windows headers
// Require Vista or later
#ifndef WINVER
#define WINVER 0x0600 
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#ifndef NTDDI_VERSION
#define NTDDI_VERSION 0x06000000
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <mmsystem.h>
#include <cfloat>
#include <wrl\client.h>

// Direct3D Includes
#include <d3d11_1.h>
#include <dxgi1_2.h>
#include <d3dx9math.h>

#ifndef DXGI_ADAPTER_FLAG_SOFTWARE
#define DXGI_ADAPTER_FLAG_SOFTWARE 2
#endif

#undef interface

#endif
