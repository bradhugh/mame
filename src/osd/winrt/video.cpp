// license:BSD-3-Clause
// copyright-holders:Brad Hughes, Aaron Giles
//============================================================
//
//  video.cpp - Win32RT video handling
//
//============================================================

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dxgi1_2.h>
#include <wrl\client.h>
#undef interface

// MAME headers
#include "emu.h"
#include "ui/ui.h"
#include "emuopts.h"
#include "render.h"
#include "uiinput.h"

// MAMEOS headers
#include "winrtmain.h"
#include "video.h"
#include "window.h"
#include "strconv.h"

#include "modules/osdwindow.h"

using namespace Platform;
using namespace Microsoft::WRL;

//============================================================
//  GLOBAL VARIABLES
//============================================================

osd_video_config video_config;

// monitor info
osd_monitor_info *osd_monitor_info::list = NULL;

//============================================================
//  PROTOTYPES
//============================================================

static void init_monitors(void);

static void check_osd_inputs(running_machine &machine);

static float get_aspect(const char *defdata, const char *data, int report_error);
static void get_resolution(const char *defdata, const char *data, osd_window_config *config, int report_error);

//============================================================
//  video_init
//============================================================

// FIXME: Temporary workaround
static osd_window_config   windows[MAX_WINDOWS];        // configuration data per-window

bool winrt_osd_interface::video_init()
{
	// extract data from the options
	extract_video_config();

	// set up monitors first
	init_monitors();

	// initialize the window system so we can make windows
	window_init();

	// create the windows
	winrt_options &options = downcast<winrt_options &>(machine().options());
	for (int index = 0; index < video_config.numscreens; index++)
	{
		winrt_window_info::create(machine(), index, osd_monitor_info::pick_monitor(options, index), &windows[index]);
	}

	if (video_config.mode != VIDEO_MODE_NONE)
		win_window_list->m_window->Activate();

	return true;
}

//============================================================
//  extract_video_config
//============================================================

void winrt_osd_interface::extract_video_config()
{
	const char *stemp;

	// global options: extract the data
	video_config.windowed = options().window();
	video_config.prescale = options().prescale();
	video_config.filter = options().filter();
	video_config.keepaspect = options().keep_aspect();
	video_config.numscreens = options().numscreens();
	video_config.fullstretch = options().uneven_stretch();

	// if we are in debug mode, never go full screen
	if (machine().debug_flags & DEBUG_FLAG_OSD_ENABLED)
		video_config.windowed = TRUE;

	// per-window options: extract the data
	const char *default_resolution = options().resolution();
	get_resolution(default_resolution, options().resolution(0), &windows[0], TRUE);
	get_resolution(default_resolution, options().resolution(1), &windows[1], TRUE);
	get_resolution(default_resolution, options().resolution(2), &windows[2], TRUE);
	get_resolution(default_resolution, options().resolution(3), &windows[3], TRUE);

	// video options: extract the data
	stemp = options().video();
	if (strcmp(stemp, "auto") == 0)
		video_config.mode = VIDEO_MODE_BGFX;
	else if (strcmp(stemp, "bgfx") == 0)
		video_config.mode = VIDEO_MODE_BGFX;
	else if (strcmp(stemp, "none") == 0)
	{
		video_config.mode = VIDEO_MODE_NONE;
		if (options().seconds_to_run() == 0)
			osd_printf_warning("Warning: -video none doesn't make much sense without -seconds_to_run\n");
	}
	else
	{
		osd_printf_warning("Invalid video value %s; reverting to gdi\n", stemp);
		video_config.mode = VIDEO_MODE_GDI;
	}
	video_config.waitvsync = options().wait_vsync();
	video_config.syncrefresh = options().sync_refresh();
	video_config.triplebuf = true;
	video_config.switchres = options().switch_res();

	if (video_config.prescale < 1 || video_config.prescale > 3)
	{
		osd_printf_warning("Invalid prescale option, reverting to '1'\n");
		video_config.prescale = 1;
	}
}

winrt_monitor_info::winrt_monitor_info(const HMONITOR handle, const char *monitor_device, float aspect, IDXGIOutput *output)
	: osd_monitor_info(handle, monitor_device, aspect), m_output(output)
{
	winrt_monitor_info::refresh();
}

//============================================================
//  init_monitors
//============================================================

static void init_monitors(void)
{
	osd_monitor_info **tailptr;

	// make a list of monitors
	osd_monitor_info::list = NULL;
	tailptr = &osd_monitor_info::list;

	ComPtr<IDXGIDevice> dxgiDevice;
	ComPtr<IDXGIFactory2> factory;
	ComPtr<IDXGIAdapter> adapter;

	// TODO: Check return
	CreateDXGIFactory1(__uuidof(IDXGIFactory2), reinterpret_cast<void**>(factory.GetAddressOf()));

	UINT iAdapter = 0;
	while (!factory->EnumAdapters(iAdapter, adapter.ReleaseAndGetAddressOf()))
	{
		UINT i = 0;
		ComPtr<IDXGIOutput> output;
		DXGI_OUTPUT_DESC desc;
		osd_monitor_info *monitor;
		while (!adapter->EnumOutputs(i, output.GetAddressOf()))
		{
			output->GetDesc(&desc);

			// guess the aspect ratio assuming square pixels
			RECT * coords = &desc.DesktopCoordinates;
			float aspect = (float)(coords->right - coords->left) / (float)(coords->bottom - coords->top);

			// allocate a new monitor info
			char *temp = utf8_from_wstring(desc.DeviceName);

			// copy in the data
			monitor = global_alloc(winrt_monitor_info(desc.Monitor, temp, aspect, output.Detach()));
			osd_free(temp);

			// hook us into the list
			*tailptr = monitor;
			tailptr = &monitor->m_next;

			i++;
		}

		// if we're verbose, print the list of monitors
		{
			osd_monitor_info *monitor;
			for (monitor = osd_monitor_info::list; monitor != NULL; monitor = monitor->m_next)
			{
				osd_printf_verbose("Video: Monitor %p = \"%s\" %s\n", monitor->oshandle(), monitor->devicename(), monitor->is_primary() ? "(primary)" : "");
			}
		}

		iAdapter++;
	}
}

winrt_monitor_info::~winrt_monitor_info()
{
}

//============================================================
//  winvideo_monitor_refresh
//============================================================

void winrt_monitor_info::refresh()
{
	DXGI_OUTPUT_DESC desc;
	m_output->GetDesc(&desc);

	// fetch the latest info about the monitor
	char *temp = utf8_from_wstring(desc.DeviceName);

	if (temp) strncpy(m_name, temp, sizeof(m_name));

	osd_free(temp);

	m_pos_size = RECT_to_osd_rect(desc.DesktopCoordinates);
	m_usuable_pos_size = RECT_to_osd_rect(desc.DesktopCoordinates);
	m_is_primary = desc.AttachedToDesktop;
}

//============================================================
//  pick_monitor
//============================================================

osd_monitor_info *osd_monitor_info::pick_monitor(osd_options &osdopts, int index)
{
	winrt_options &options = reinterpret_cast<winrt_options &>(osdopts);
	osd_monitor_info *monitor;
	const char *scrname, *scrname2;
	int moncount = 0;
	float aspect;

	// get the screen option
	scrname = options.screen();
	scrname2 = options.screen(index);

	// decide which one we want to use
	if (strcmp(scrname2, "auto") != 0)
		scrname = scrname2;

	// get the aspect ratio
	aspect = get_aspect(options.aspect(), options.aspect(index), TRUE);

	// look for a match in the name first
	if (scrname != NULL && (scrname[0] != 0))
	{
		for (monitor = osd_monitor_info::list; monitor != NULL; monitor = monitor->next())
		{
			moncount++;
			if (strcmp(scrname, monitor->devicename()) == 0)
				goto finishit;
		}
	}

	// didn't find it; alternate monitors until we hit the jackpot
	index %= moncount;
	for (monitor = osd_monitor_info::list; monitor != NULL; monitor = monitor->next())
		if (index-- == 0)
			goto finishit;

	// return the primary just in case all else fails
	for (monitor = osd_monitor_info::list; monitor != NULL; monitor = monitor->next())
		if (monitor->is_primary())
			goto finishit;

	// FIXME: FatalError?
finishit:
	if (aspect != 0)
	{
		monitor->set_aspect(aspect);
	}
	return monitor;
}

//============================================================
//  sdlvideo_monitor_get_aspect
//============================================================

float osd_monitor_info::aspect()
{
	// FIXME: returning 0 looks odd, video_config is bad
	if (video_config.keepaspect)
	{
		return m_aspect / ((float)m_pos_size.width() / (float)m_pos_size.height());
	}
	return 0.0f;
}

//============================================================
//  get_aspect
//============================================================

static float get_aspect(const char *defdata, const char *data, int report_error)
{
	int num = 0, den = 1;

	if (strcmp(data, OSDOPTVAL_AUTO) == 0)
	{
		if (strcmp(defdata, OSDOPTVAL_AUTO) == 0)
			return 0;
		data = defdata;
	}
	if (sscanf(data, "%d:%d", &num, &den) != 2 && report_error)
		osd_printf_error("Illegal aspect ratio value = %s\n", data);
	return (float)num / (float)den;
}

//============================================================
//  get_resolution
//============================================================

static void get_resolution(const char *defdata, const char *data, osd_window_config *config, int report_error)
{
	config->width = config->height = config->depth = config->refresh = 0;
	if (strcmp(data, OSDOPTVAL_AUTO) == 0)
	{
		if (strcmp(defdata, OSDOPTVAL_AUTO) == 0)
			return;
		data = defdata;
	}

	if (sscanf(data, "%dx%dx%d", &config->width, &config->height, &config->depth) < 2 && report_error)
		osd_printf_error("Illegal resolution value = %s\n", data);

	const char * at_pos = strchr(data, '@');
	if (at_pos)
		if (sscanf(at_pos + 1, "%d", &config->refresh) < 1 && report_error)
			osd_printf_error("Illegal refresh rate in resolution value = %s\n", data);
}

//============================================================
//  winvideo_monitor_from_handle
//============================================================

osd_monitor_info *winrt_monitor_info::monitor_from_handle(HMONITOR hmonitor)
{
	osd_monitor_info *monitor;

	// find the matching monitor
	for (monitor = osd_monitor_info::list; monitor != NULL; monitor = monitor->m_next)
		if (*((HMONITOR *)monitor->oshandle()) == hmonitor)
			return monitor;
	return NULL;
}

//============================================================
//  update
//============================================================

void winrt_osd_interface::update(bool skip_redraw)
{
	update_slider_list();

	// if we're not skipping this redraw, update all windows
	if (!skip_redraw)
	{
		//      profiler_mark(PROFILER_BLIT);
		for (winrt_window_info *window = win_window_list; window != NULL; window = window->m_next)
			window->update();
		//      profiler_mark(PROFILER_END);
	}

	// poll the joystick values here
	winwindow_process_events(machine(), TRUE, FALSE);
	poll_input(machine());
	check_osd_inputs(machine());
	// if we're running, disable some parts of the debugger
	if ((machine().debug_flags & DEBUG_FLAG_OSD_ENABLED) != 0)
		debugger_update();
}

//============================================================
//  check_osd_inputs
//============================================================

static void check_osd_inputs(running_machine &machine)
{
	// TODO: Fix
	// check for toggling fullscreen mode
	//if (machine.ui_input().pressed(IPT_OSD_1))
	//	winwindow_toggle_full_screen();

	//// check for taking fullscreen snap
	//if (machine.ui_input().pressed(IPT_OSD_2))
	//	winwindow_take_snap();

	//// check for taking fullscreen video
	//if (machine.ui_input().pressed(IPT_OSD_3))
	//	winwindow_take_video();

	//// check for taking fullscreen video
	//if (machine.ui_input().pressed(IPT_OSD_4))
	//	winwindow_toggle_fsfx();
}

//============================================================
//  get_slider_list
//============================================================

slider_state *winrt_osd_interface::get_slider_list()
{
	return m_sliders;
}