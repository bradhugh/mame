// license:BSD-3-Clause
// copyright-holders:Aaron Giles, Brad Hughes
//============================================================
//
//  winfile.c - WinRT OSD core file access functions
//
//============================================================

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winioctl.h>
#include <tchar.h>
#include <stdlib.h>
#include <ctype.h>

// MAME headers
#include "osdcore.h"

// MAMEOS headers
#include "strconv.h"
#include "../windows/winutil.h"

#include "winrtfile.h"

//============================================================
//  FUNCTION PROTOTYPES
//============================================================

static DWORD create_path_recursive(const TCHAR *path);

//============================================================
//  INLINE FUNCTIONS
//============================================================

static inline int is_path_to_physical_drive(const char *path)
{
	return (_strnicmp(path, "\\\\.\\physicaldrive", 17) == 0);
}

extern const char *winfile_ptty_identifier;


//============================================================
//  osd_open
//============================================================

file_error osd_open(const char *orig_path, UINT32 openflags, osd_file **file, UINT64 *filesize)
{
	file_error filerr = FILERR_NONE;
	char *path = nullptr;

	osd_subst_env(&path, orig_path);

	// convert path to TCHAR
	TCHAR *t_path = tstring_from_utf8(path);
	if (t_path == NULL)
	{
		filerr = FILERR_OUT_OF_MEMORY;
		goto error;
	}

	// allocate a file object, plus space for the converted filename
	*file = (osd_file *)osd_malloc_array(sizeof(**file) + sizeof(TCHAR) * _tcslen(t_path));
	if (*file == NULL)
	{
		filerr = FILERR_OUT_OF_MEMORY;
		goto error;
	}
	memset(*file, 0x00, sizeof(**file) + sizeof(TCHAR) * _tcslen(t_path));

	if (win_check_socket_path(path))
	{
		(*file)->type = WINFILE_SOCKET;
		filerr = win_open_socket(path, openflags, file, filesize);
		goto error;
	}

	if (strncmp(path, winfile_ptty_identifier, strlen(winfile_ptty_identifier)) == 0)
	{
		(*file)->type = WINFILE_PTTY;
		filerr = win_open_ptty(path, openflags, file, filesize);
		goto error;
	}

	(*file)->type = WINFILE_FILE;

	// convert the path into something Windows compatible
	{
		TCHAR *dst = (*file)->filename;
		for (TCHAR const *src = t_path; *src != 0; src++)
			*dst++ = *src;//(*src == '/') ? '\\' : *src;
		*dst++ = 0;
	}

	// select the file open modes
	DWORD disposition, access, sharemode;
	if (openflags & OPEN_FLAG_WRITE)
	{
		disposition = (!is_path_to_physical_drive(path) && (openflags & OPEN_FLAG_CREATE)) ? CREATE_ALWAYS : OPEN_EXISTING;
		access = (openflags & OPEN_FLAG_READ) ? (GENERIC_READ | GENERIC_WRITE) : GENERIC_WRITE;
		sharemode = FILE_SHARE_READ;
	}
	else if (openflags & OPEN_FLAG_READ)
	{
		disposition = OPEN_EXISTING;
		access = GENERIC_READ;
		sharemode = FILE_SHARE_READ;
	}
	else
	{
		filerr = FILERR_INVALID_ACCESS;
		goto error;
	}

	// attempt to open the file
	(*file)->handle = CreateFile2((*file)->filename, access, sharemode, disposition, nullptr);
	if ((*file)->handle == INVALID_HANDLE_VALUE)
	{
		DWORD error = GetLastError();
		// create the path if necessary
		if (error == ERROR_PATH_NOT_FOUND && (openflags & OPEN_FLAG_CREATE) && (openflags & OPEN_FLAG_CREATE_PATHS))
		{
			TCHAR *pathsep = _tcsrchr((*file)->filename, '\\');
			if (pathsep != NULL)
			{
				// create the path up to the file
				*pathsep = 0;
				error = create_path_recursive((*file)->filename);
				*pathsep = '\\';

				// attempt to reopen the file
				if (error == NO_ERROR)
					(*file)->handle = CreateFile2((*file)->filename, access, sharemode, disposition, nullptr);
			}
		}

		// if we still failed, clean up and free
		if ((*file)->handle == INVALID_HANDLE_VALUE)
		{
			filerr = win_error_to_file_error(error);
			goto error;
		}
	}

	// get the file size
	FILE_STANDARD_INFO file_info;
	GetFileInformationByHandleEx((*file)->handle, FileStandardInfo, &file_info, sizeof(file_info));
	*filesize = file_info.EndOfFile.QuadPart;

error:
	// cleanup
	if (filerr != FILERR_NONE && *file != NULL)
	{
		osd_free(*file);
		*file = NULL;
	}
	osd_free(t_path);
	osd_free(path);
	return filerr;
}


//============================================================
//  osd_read
//============================================================

file_error osd_read(osd_file *file, void *buffer, UINT64 offset, UINT32 length, UINT32 *actual)
{
	_LARGE_INTEGER largeOffset;
	largeOffset.QuadPart = offset;
	DWORD result;

	switch (file->type)
	{
		case WINFILE_FILE:
			// attempt to set the file pointer
			if (!SetFilePointerEx(file->handle, largeOffset, NULL, FILE_BEGIN))
			{
				DWORD error = GetLastError();
				if (error != NO_ERROR)
					return win_error_to_mame_file_error(error);
			}

			// then perform the read
			if (!ReadFile(file->handle, buffer, length, &result, NULL))
				return win_error_to_mame_file_error(GetLastError());
			if (actual != NULL)
				*actual = result;
			break;
		case WINFILE_SOCKET:
			return win_read_socket(file, buffer, offset, length, actual);
		case WINFILE_PTTY:
			return win_read_ptty(file, buffer, offset, length, actual);

	}
	return FILERR_NONE;
}


//============================================================
//  osd_write
//============================================================

file_error osd_write(osd_file *file, const void *buffer, UINT64 offset, UINT32 length, UINT32 *actual)
{
	_LARGE_INTEGER largeOffset;
	largeOffset.QuadPart = offset;
	DWORD result;

	switch (file->type)
	{
		case WINFILE_FILE:
			// attempt to set the file pointer
			if (!SetFilePointerEx(file->handle, largeOffset, NULL, FILE_BEGIN))
			{
				DWORD error = GetLastError();
				if (error != NO_ERROR)
					return win_error_to_mame_file_error(error);
			}

			// then perform the read
			if (!WriteFile(file->handle, buffer, length, &result, NULL))
				return win_error_to_mame_file_error(GetLastError());
			if (actual != NULL)
				*actual = result;
			break;
		case WINFILE_SOCKET:
			return win_write_socket(file, buffer, offset, length, actual);
		case WINFILE_PTTY:
			return win_write_ptty(file, buffer, offset, length, actual);

	}
	return FILERR_NONE;
}

//============================================================
//  osd_openpty
//============================================================

file_error osd_openpty(osd_file **file, char *name, size_t name_len)
{
	return FILERR_FAILURE;
}

//============================================================
//  osd_truncate
//============================================================

file_error osd_truncate(osd_file *file, UINT64 offset)
{
	if (!file || !file->handle)
		return FILERR_FAILURE;

	_LARGE_INTEGER largeOffset;
	largeOffset.QuadPart = offset;

	switch (file->type)
	{
		case WINFILE_FILE:
			// attempt to set the file pointer
			if (!SetFilePointerEx(file->handle, largeOffset, NULL, FILE_BEGIN))
			{
				DWORD error = GetLastError();
				if (error != NO_ERROR)
					return win_error_to_mame_file_error(error);
			}

			// then perform the truncation
			if (!SetEndOfFile(file->handle))
				return win_error_to_mame_file_error(GetLastError());
			break;
		case WINFILE_SOCKET:
			return FILERR_FAILURE;
		case WINFILE_PTTY:
			return FILERR_FAILURE;

	}
	return FILERR_NONE;
}

//============================================================
//  osd_fflush
//============================================================

file_error osd_fflush(osd_file *file)
{
	if (!file || !file->handle)
		return FILERR_FAILURE;

	switch (file->type)
	{
		case WINFILE_FILE:
			// attempt to flush file buffers
			if (!FlushFileBuffers(file->handle))
				return win_error_to_mame_file_error(GetLastError());
			break;
		case WINFILE_SOCKET:
			return FILERR_FAILURE;
		case WINFILE_PTTY:
			return FILERR_FAILURE;

	}
	return FILERR_NONE;
}


//============================================================
//  osd_close
//============================================================

file_error osd_close(osd_file *file)
{
	file_error result = FILERR_NONE;

	switch (file->type)
	{
		case WINFILE_FILE:
			CloseHandle(file->handle);
			break;
		case WINFILE_SOCKET:
			result = win_close_socket(file);
			break;
		case WINFILE_PTTY:
			result = win_close_ptty(file);
			break;
	}

	osd_free(file);
	return result;
}


//============================================================
//  osd_rmfile
//============================================================

file_error osd_rmfile(const char *filename)
{
	file_error filerr = FILERR_NONE;

	TCHAR *tempstr = tstring_from_utf8(filename);
	if (!tempstr)
	{
		filerr = FILERR_OUT_OF_MEMORY;
		goto done;
	}

	if (!DeleteFile(tempstr))
	{
		filerr = win_error_to_file_error(GetLastError());
		goto done;
	}

done:
	if (tempstr)
		osd_free(tempstr);
	return filerr;
}


//============================================================
//  osd_get_physical_drive_geometry
//============================================================

int osd_get_physical_drive_geometry(const char *filename, UINT32 *cylinders, UINT32 *heads, UINT32 *sectors, UINT32 *bps)
{
	return FALSE;
}


//============================================================
//  create_path_recursive
//============================================================

DWORD create_path_recursive(const TCHAR *path)
{
	TCHAR *sep = (TCHAR *)_tcsrchr(path, '\\');

	// if there's still a separator, and it's not the root, nuke it and recurse
	if (sep != NULL && sep > path && sep[0] != ':' && sep[-1] != '\\')
	{
		*sep = 0;
		create_path_recursive(path);
		*sep = '\\';
	}

	// if the path already exists, we're done
	WIN32_FILE_ATTRIBUTE_DATA fileInfo;
	if (GetFileAttributesEx(path, GetFileExInfoStandard, &fileInfo))
		return NO_ERROR;

	// create the path
	if (CreateDirectory(path, NULL) == 0)
		return GetLastError();
	return NO_ERROR;
}

//============================================================
//  win_error_to_mame_file_error
//============================================================

file_error win_error_to_mame_file_error(DWORD error)
{
	file_error filerr;

	// convert a Windows error to a file_error
	switch (error)
	{
		case ERROR_SUCCESS:
			filerr = FILERR_NONE;
			break;

		case ERROR_OUTOFMEMORY:
			filerr = FILERR_OUT_OF_MEMORY;
			break;

		case ERROR_FILE_NOT_FOUND:
		case ERROR_PATH_NOT_FOUND:
			filerr = FILERR_NOT_FOUND;
			break;

		case ERROR_ACCESS_DENIED:
			filerr = FILERR_ACCESS_DENIED;
			break;

		case ERROR_SHARING_VIOLATION:
			filerr = FILERR_ALREADY_OPEN;
			break;

		default:
			filerr = FILERR_FAILURE;
			break;
	}
	return filerr;
}

//============================================================
//  osd_stat
//============================================================

osd_directory_entry *osd_stat(const char *path)
{
	osd_directory_entry *result = NULL;
	TCHAR *t_path;
	HANDLE find = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA find_data;

	// convert the path to TCHARs
	t_path = tstring_from_utf8(path);
	if (t_path == NULL)
		goto done;

	// is this path a root directory (e.g. - C:)?
	if (isalpha(path[0]) && (path[1] == ':') && (path[2] == '\0'))
	{
		// need to do special logic for root directories
		memset(&find_data, 0, sizeof(find_data));
		GetFileAttributesEx(t_path, GetFileExInfoStandard, &find_data.dwFileAttributes);
	}
	else
	{
		// attempt to find the first file
		find = FindFirstFileEx(t_path, FindExInfoStandard, &find_data, FindExSearchNameMatch, NULL, 0);
		if (find == INVALID_HANDLE_VALUE)
			goto done;
	}

	// create an osd_directory_entry; be sure to make sure that the caller can
	// free all resources by just freeing the resulting osd_directory_entry
	result = (osd_directory_entry *)osd_malloc_array(sizeof(*result) + strlen(path) + 1);
	if (!result)
		goto done;
	strcpy(((char *) result) + sizeof(*result), path);
	result->name = ((char *) result) + sizeof(*result);
	result->type = win_attributes_to_entry_type(find_data.dwFileAttributes);
	result->size = find_data.nFileSizeLow | ((UINT64) find_data.nFileSizeHigh << 32);

done:
	if (t_path != NULL)
		osd_free(t_path);
	return result;
}

//============================================================
//  osd_get_full_path
//============================================================

file_error osd_get_full_path(char **dst, const char *path)
{
	file_error err;
	TCHAR *t_path;
	TCHAR buffer[MAX_PATH];

	// convert the path to TCHARs
	t_path = tstring_from_utf8(path);
	if (t_path == NULL)
	{
		err = FILERR_OUT_OF_MEMORY;
		goto done;
	}

	// cannonicalize the path
	if (!GetFullPathName(t_path, ARRAY_LENGTH(buffer), buffer, NULL))
	{
		err = win_error_to_mame_file_error(GetLastError());
		goto done;
	}

	// convert the result back to UTF-8
	*dst = utf8_from_tstring(buffer);
	if (!*dst)
	{
		err = FILERR_OUT_OF_MEMORY;
		goto done;
	}

	err = FILERR_NONE;

done:
	if (t_path != NULL)
		osd_free(t_path);
	return err;
}

//============================================================
//  osd_get_volume_name
//============================================================

const char *osd_get_volume_name(int idx)
{
	return nullptr;
}
