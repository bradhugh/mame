// license:BSD-3-Clause
// copyright-holders:Brad Hughes
//============================================================
//
//  winrtcompat.h - WinRT C-Runtime forced include
//
//============================================================

#ifndef __WINRT_CRT_COMPAT_H__
#define __WINRT_CRT_COMPAT_H__

#ifdef __cplusplus
extern "C" {
#endif

	int system(const char *command);

	const char *getenv(const char *varname);

#ifdef __cplusplus
}
#endif

#endif // __WINRT_CRT_COMPAT_H__


