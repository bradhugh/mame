// license:BSD-3-Clause
// copyright-holders:Brad Hughes
//============================================================
//
//  video.h - Windows RT implementation of MAME video routines
//
//============================================================

#ifndef __WINRT_VIDEO__
#define __WINRT_VIDEO__

#include "render.h"
#include "modules/osdwindow.h"

//============================================================
//  CONSTANTS
//============================================================

#define MAX_WINDOWS         1

//============================================================
//  TYPE DEFINITIONS
//============================================================

inline osd_rect RECT_to_osd_rect(const RECT &r)
{
	return osd_rect(r.left, r.top, r.right - r.left, r.bottom - r.top);
}

struct dxgi_monitor_info
{
	WCHAR DeviceName[32];
	RECT DesktopCoordinates;
	BOOL AttachedToDesktop;
	HMONITOR Monitor;
};

struct IDXGIOutput;

class winrt_monitor_info : public osd_monitor_info
{
public:
	winrt_monitor_info(const HMONITOR handle, const char *monitor_device, float aspect, IDXGIOutput* output);
	virtual ~winrt_monitor_info();

	virtual void refresh() override;

	// static
	static osd_monitor_info *monitor_from_handle(HMONITOR monitor);

private:
	IDXGIOutput *       m_output;                 // The output interface

	//dxgi_monitor_info	m_info;                   // monitor info
};

struct osd_video_config
{
	// global configuration
	int                 windowed;                   // start windowed?
	int                 prescale;                   // prescale factor
	int                 keepaspect;                 // keep aspect ratio
	int                 numscreens;                 // number of screens

	// hardware options
	int                 mode;                       // output mode
	int                 waitvsync;                  // spin until vsync
	int                 syncrefresh;                // sync only to refresh rate
	int                 switchres;                  // switch resolutions

	int                 fullstretch;    // FXIME: implement in windows!

	// d3d, accel, opengl
	int                 filter;                     // enable filtering

	// dd, d3d
	int                 triplebuf;                  // triple buffer

	// vector options
	float               beamwidth;      // beam width

	// perftest
	int                 perftest;       // print out real video fps

	// YUV options
	int                 scale_mode;
};

//============================================================
//  GLOBAL VARIABLES
//============================================================

extern osd_video_config video_config;

#endif