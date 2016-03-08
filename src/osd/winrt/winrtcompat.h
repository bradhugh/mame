// license:BSD-3-Clause
// copyright-holders:Brad Hughes
//============================================================
//
//  winrtcompat.h - Windows runtime compat forced includes
//
//============================================================

#ifndef __WINRTCOMPAT_H__
#define __WINRTCOMPAT_H__

int __cdecl system(const char *command);
char *getenv(const char *varname);

void* __stdcall CreateFileA(
	const char *        lpFileName,
	int                 dwDesiredAccess,
	int                 dwShareMode,
	void*               lpSecurityAttributes,
	int                 dwCreationDisposition,
	int                 dwFlagsAndAttributes,
	void*               hTemplateFile);

void* __stdcall CreateFileW(
	void *              lpFileName,
	int                 dwDesiredAccess,
	int                 dwShareMode,
	void*               lpSecurityAttributes,
	int                 dwCreationDisposition,
	int                 dwFlagsAndAttributes,
	void*               hTemplateFile);

void* __stdcall CreateFile(
	const char *        lpFileName,
	int                 dwDesiredAccess,
	int                 dwShareMode,
	void*               lpSecurityAttributes,
	int                 dwCreationDisposition,
	int                 dwFlagsAndAttributes,
	void*               hTemplateFile);

#if defined(UNICODE) && __cplusplus
void* __stdcall CreateFile(
	wchar_t *           lpFileName,
	int                 dwDesiredAccess,
	int                 dwShareMode,
	void*               lpSecurityAttributes,
	int                 dwCreationDisposition,
	int                 dwFlagsAndAttributes,
	void*               hTemplateFile);
#endif

#endif // __WINRTCOMPAT_H__


