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

class winrt_monitor_info : public osd_monitor_info
{
public:
	winrt_monitor_info(float aspect);
	virtual ~winrt_monitor_info();

	virtual void refresh() override;
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