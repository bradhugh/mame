// license:BSD-3-Clause
// copyright-holders:Brad Hughes
//============================================================
//
//  winrtcompat.h - Windows runtime compat forced includes
//
//============================================================

#ifndef __WINRTCOMPAT_H__
#define __WINRTCOMPAT_H__

#ifdef __cplusplus
extern "C" {
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

HGLOBAL
WINAPI
GlobalAlloc(
	_In_ UINT uFlags,
	_In_ SIZE_T dwBytes
	);

HGLOBAL
WINAPI
GlobalFree(
	_In_ HGLOBAL hMem
	);

HANDLE
WINAPI
CreateFileA(
	_In_ LPCSTR lpFileName,
	_In_ DWORD dwDesiredAccess,
	_In_ DWORD dwShareMode,
	_In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	_In_ DWORD dwCreationDisposition,
	_In_ DWORD dwFlagsAndAttributes,
	_In_opt_ HANDLE hTemplateFile
	);

HANDLE
WINAPI
CreateFileW(
	_In_ LPCWSTR lpFileName,
	_In_ DWORD dwDesiredAccess,
	_In_ DWORD dwShareMode,
	_In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	_In_ DWORD dwCreationDisposition,
	_In_ DWORD dwFlagsAndAttributes,
	_In_opt_ HANDLE hTemplateFile
	);

#ifdef UNICODE
#define CreateFile  CreateFileW
#else
#define CreateFile  CreateFileA
#endif // !UNICODE


DWORD WINAPI GetTickCount(void);

FILE *_popen(
	const char *command,
	const char *mode
	);

int _pclose(
	FILE *stream);

_Ret_maybenull_
HMODULE
WINAPI
LoadLibraryExA(
	_In_ LPCSTR lpLibFileName,
	_Reserved_ HANDLE hFile,
	_In_ DWORD dwFlags
	);

_Ret_maybenull_
HMODULE
WINAPI
LoadLibraryExW(
	_In_ LPCWSTR lpLibFileName,
	_Reserved_ HANDLE hFile,
	_In_ DWORD dwFlags
	);

#ifdef UNICODE
#define LoadLibraryEx  LoadLibraryExW
#else
#define LoadLibraryEx  LoadLibraryExA
#endif // !UNICODE

#ifdef __cplusplus
}
#endif

#endif // __WINRTCOMPAT_H__


