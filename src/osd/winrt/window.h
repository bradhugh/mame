// license:BSD-3-Clause
// copyright-holders:Brad Hughes
//============================================================
//
//  window.h - Windows RT implementation of MAME window
//
//============================================================

#ifndef __WINRT_WINDOW__
#define __WINRT_WINDOW__

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>

#include <mutex>
#include "video.h"
#include "render.h"

#include "modules/osdwindow.h"

using namespace Windows::Foundation;

//============================================================
//  CONSTANTS
//============================================================

#define RESIZE_STATE_NORMAL     0
#define RESIZE_STATE_RESIZING   1
#define RESIZE_STATE_PENDING    2

//============================================================
//  TYPE DEFINITIONS
//============================================================

class winrt_window_info : public osd_window
{
public:
	winrt_window_info(running_machine &machine);
	virtual ~winrt_window_info();

	running_machine &machine() const override { return m_machine; }

	virtual render_target *target() override { return m_target; }
	int fullscreen() const override { return false; }

	void update();

	virtual osd_dim get_size() override
	{
		Rect client = m_window->Bounds;
		return osd_dim(client.Right - client.Left, client.Bottom - client.Top);
	}

	virtual osd_monitor_info *monitor() const override { return m_monitor; }

	virtual osd_monitor_info *winwindow_video_window_monitor(const osd_rect *proposed) override;

	void destroy();

	// static

	static void create(running_machine &machine, int index, osd_monitor_info *monitor, const osd_window_config *config);

	// member variables

	winrt_window_info *     m_next;
	volatile int            m_init_state;

	//// window handle and info
	//char                m_title[256];
	//RECT                m_non_fullscreen_bounds;
	//int                 m_startmaximized;
	//int                 m_isminimized;
	//int                 m_ismaximized;

	//// monitor info
	osd_monitor_info *  m_monitor;
	//float               m_aspect;

	// rendering info
	std::mutex          m_render_lock;
	render_target *     m_target;
	int                 m_targetview;
	int                 m_targetorient;
	render_layer_config m_targetlayerconfig;

	// input info
	DWORD               m_lastclicktime;
	int                 m_lastclickx;
	int                 m_lastclicky;

	// drawing data
	osd_renderer *      m_renderer;

private:
	void draw_video_contents(HDC dc, int update);
	int complete_create();
	void set_starting_view(int index, const char *defview, const char *view);
	int wnd_extra_width();
	int wnd_extra_height();
	osd_rect constrain_to_aspect_ratio(const osd_rect &rect, int adjustment);
	osd_dim get_min_bounds(int constrain);
	osd_dim get_max_bounds(int constrain);
	void adjust_window_position_after_major_change();

	running_machine &   m_machine;
};

//============================================================
//  GLOBAL VARIABLES
//============================================================

// windows
extern winrt_window_info *win_window_list;

//============================================================
//  rect_width / rect_height
//============================================================

static inline int rect_width(const RECT *rect)
{
	return rect->right - rect->left;
}


static inline int rect_height(const RECT *rect)
{
	return rect->bottom - rect->top;
}
#endif