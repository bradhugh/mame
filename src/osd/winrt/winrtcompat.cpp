// license:BSD-3-Clause
// copyright-holders:Brad Hughes
//============================================================
//
//  winrtcompat.cpp - Windows runtime compat functions
//
//============================================================

#include "winrtcompat.h"

#include <errno.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

int __cdecl system(const char *command)
{
	return ENOENT;
}

char *getenv(const char *varname)
{
	return nullptr;
}

void* __stdcall CreateFileW(
	void *              lpFileName,
	int                 dwDesiredAccess,
	int                 dwShareMode,
	void*               lpSecurityAttributes,
	int                 dwCreationDisposition,
	int                 dwFlagsAndAttributes,
	void*               hTemplateFile)
{
	// TODO: Handle other arguments that go into last param (pCreateExParams)
	return CreateFile2((wchar_t*)lpFileName, dwDesiredAccess, dwShareMode, dwCreationDisposition, NULL);
}

void* __stdcall CreateFileA(
	const char *        lpFileName,
	int                 dwDesiredAccess,
	int                 dwShareMode,
	void*				lpSecurityAttributes,
	int                 dwCreationDisposition,
	int                 dwFlagsAndAttributes,
	void*               hTemplateFile)
{
	wchar_t filepath[MAX_PATH + 1];
	if (MultiByteToWideChar(CP_ACP, 0, lpFileName, strlen(lpFileName), filepath, MAX_PATH))
		return CreateFileW(filepath, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);

	SetLastError(E_FAIL);
	return INVALID_HANDLE_VALUE;
}

void* __stdcall CreateFile(
	const char *        lpFileName,
	int                 dwDesiredAccess,
	int                 dwShareMode,
	void*               lpSecurityAttributes,
	int                 dwCreationDisposition,
	int                 dwFlagsAndAttributes,
	void*               hTemplateFile)
{
	return CreateFileA(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

#if defined(UNICODE)
void* __stdcall CreateFile(
	void *              lpFileName,
	int                 dwDesiredAccess,
	int                 dwShareMode,
	void*               lpSecurityAttributes,
	int                 dwCreationDisposition,
	int                 dwFlagsAndAttributes,
	void*               hTemplateFile)
{
	return CreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}
#endif
