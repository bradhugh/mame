// license:BSD-3-Clause
// copyright-holders:Aaron Giles
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <stdlib.h>

#include "winrtfile.h"
#include "strconv.h"
#include "../windows/winutil.h"

const char *winfile_ptty_identifier = "\\\\.\\pipe\\";

file_error win_open_ptty(const char *path, UINT32 openflags, osd_file **file, UINT64 *filesize)
{
	return FILERR_ACCESS_DENIED;
}

file_error win_read_ptty(osd_file *file, void *buffer, UINT64 offset, UINT32 count, UINT32 *actual)
{
	BOOL res;
	DWORD bytes_read;

	res = ReadFile(file->handle, buffer, count, &bytes_read, NULL);
	if(res == FALSE)
		return win_error_to_file_error(GetLastError());

	if(actual != NULL)
		*actual = bytes_read;

	return FILERR_NONE;
}

file_error win_write_ptty(osd_file *file, const void *buffer, UINT64 offset, UINT32 count, UINT32 *actual)
{
	BOOL res;
	DWORD bytes_wrote;

	res = WriteFile(file->handle, buffer, count, &bytes_wrote, NULL);
	if(res == FALSE)
		return win_error_to_file_error(GetLastError());

	if(actual != NULL)
		*actual = bytes_wrote;

	return FILERR_NONE;
}

file_error win_close_ptty(osd_file *file)
{
	return FILERR_NONE;
}
