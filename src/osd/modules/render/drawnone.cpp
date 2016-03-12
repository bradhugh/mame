// license:BSD-3-Clause
// copyright-holders:Aaron Giles
//============================================================
//
//  drawnone.cpp - stub "nothing" drawer
//
//============================================================

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// MAME headers
#include "emu.h"

#include "drawnone.h"

//============================================================
//  drawnone_window_get_primitives
//============================================================

render_primitive_list *renderer_none::get_primitives()
{
	RECT client;
#if defined(OSD_WINDOWS)
	GetClientRect(window().m_hwnd, &client);
#elif defined(OSD_WINRT)
	auto bounds = window().m_window->Bounds;
	client.left = bounds.Left;
	client.right = bounds.Right;
	client.top = bounds.Top;
	client.bottom = bounds.Bottom;
#endif
	window().target()->set_bounds(rect_width(&client), rect_height(&client), window().aspect());
	return &window().target()->get_primitives();
}
