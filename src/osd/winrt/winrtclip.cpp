// license:BSD-3-Clause
// copyright-holders:Aaron Giles
//============================================================
//
//  winclip.c - Win32 OSD core clipboard access functions
//
//============================================================

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wrl\client.h>

#include "strconv.h"

using namespace Platform;
using namespace Windows::ApplicationModel::DataTransfer;
using namespace Windows::Foundation;

//============================================================
//  get_clipboard_text_by_format
//============================================================

static char *get_clipboard_text_by_format(UINT format, char *(*convert)(LPCVOID data))
{
	DataPackageView^ dataPackageView;
	IAsyncOperation<String^>^ getTextOp;
	String^ clipboardText;

	dataPackageView = Clipboard::GetContent();
	getTextOp = dataPackageView->GetTextAsync();
	clipboardText = getTextOp->GetResults();

	return utf8_from_wstring(clipboardText->Data());
}

//============================================================
//  convert_wide
//============================================================

static char *convert_wide(LPCVOID data)
{
	return utf8_from_wstring((LPCWSTR) data);
}

//============================================================
//  convert_ansi
//============================================================

static char *convert_ansi(LPCVOID data)
{
	return utf8_from_astring((LPCSTR) data);
}



//============================================================
//  osd_get_clipboard_text
//============================================================

char *osd_get_clipboard_text(void)
{
	char *result;

	// try to access unicode text
	result = get_clipboard_text_by_format(CF_UNICODETEXT, convert_wide);

	// try to access ANSI text
	if (result == NULL)
		result = get_clipboard_text_by_format(CF_TEXT, convert_ansi);

	return result;
}
