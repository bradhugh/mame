// license:BSD-3-Clause
// copyright-holders:Brad Hughes
//============================================================
//
//  window.cpp - WinRT window handling
//
//============================================================

// MAME headers
#include "emu.h"
#include "uiinput.h"

// MAMEOS headers
#include "winrtmain.h"
#include "window.h"
#include "video.h"
#include "winutf8.h"

#include "winutil.h"

#include "modules/render/drawbgfx.h"
#include "modules/render/drawnone.h"

using namespace Windows::UI::Core;

//============================================================
//  GLOBAL VARIABLES
//============================================================

winrt_window_info *win_window_list;
static winrt_window_info **last_window_ptr;
static DWORD main_threadid;

// actual physical resolution
static int win_physical_width;
static int win_physical_height;

// event handling
static DWORD last_event_check;

static HANDLE window_thread;
static DWORD window_threadid;

static DWORD last_update_time;

static HANDLE ui_pause_event;
static HANDLE window_thread_ready_event;

// minimum window dimension
#define MIN_WINDOW_DIM                  200

winrt_window_info::winrt_window_info(running_machine &machine)
	: osd_window(), m_next(nullptr),
	m_init_state(0),
	m_monitor(nullptr),
	m_target(nullptr),
	m_targetview(0),
	m_targetorient(0),
	m_lastclicktime(0),
	m_lastclickx(0),
	m_lastclicky(0),
	m_renderer(nullptr),
	m_machine(machine)
{
	m_prescale = video_config.prescale;
}

winrt_window_info::~winrt_window_info()
{
	if (m_renderer != nullptr)
	{
		delete m_renderer;
	}
}

//============================================================
//  wnd_extra_width
//  (window thread)
//============================================================

int winrt_window_info::wnd_extra_width()
{
	RECT temprect = { 100, 100, 200, 200 };
	//AdjustWindowRectEx(&temprect, WINDOW_STYLE, win_has_menu(), WINDOW_STYLE_EX);
	return rect_width(&temprect) - 100;
}



//============================================================
//  wnd_extra_height
//  (window thread)
//============================================================

int winrt_window_info::wnd_extra_height()
{
	RECT temprect = { 100, 100, 200, 200 };
	//AdjustWindowRectEx(&temprect, WINDOW_STYLE, win_has_menu(), WINDOW_STYLE_EX);
	return rect_height(&temprect) - 100;
}

//============================================================
//  window_init
//  (main thread)
//============================================================

bool winrt_osd_interface::window_init()
{
	// get the main thread ID before anything else
	main_threadid = GetCurrentThreadId();

	// set up window class and register it
	// create_window_class();

	// create an event to signal UI pausing
	ui_pause_event = CreateEvent(nullptr, TRUE, FALSE, nullptr);
	if (!ui_pause_event)
		fatalerror("Failed to create pause event\n");

	// treat the window thread as the main thread
	window_thread = GetCurrentThread();
	window_threadid = main_threadid;

	const int fallbacks[VIDEO_MODE_COUNT] = {
		-1,                 // NONE -> no fallback
		VIDEO_MODE_NONE,    // GDI -> NONE
	};

	int current_mode = video_config.mode;
	while (current_mode != VIDEO_MODE_NONE)
	{
		bool error = false;
		switch (current_mode)
		{
		case VIDEO_MODE_NONE:
			error = renderer_none::init(machine());
			break;
		case VIDEO_MODE_BGFX:
			error = renderer_bgfx::init(machine());
			break;
		default:
			fatalerror("Unknown video mode.");
			break;
		}
		if (error)
		{
			current_mode = fallbacks[current_mode];
		}
		else
		{
			break;
		}
	}

	video_config.mode = current_mode;

	// set up the window list
	last_window_ptr = &win_window_list;

	return true;
}

//============================================================
//  winwindow_exit
//  (main thread)
//============================================================

void winrt_osd_interface::window_exit()
{
	assert(GetCurrentThreadId() == main_threadid);

	// free all the windows
	while (win_window_list != nullptr)
	{
		winrt_window_info *temp = win_window_list;
		win_window_list = temp->m_next;
		temp->destroy();
		global_free(temp);
	}

	switch (video_config.mode)
	{
	case VIDEO_MODE_NONE:
		renderer_none::exit();
		break;
	case VIDEO_MODE_BGFX:
		renderer_bgfx::exit();
		break;
	}

	// kill the UI pause event
	if (ui_pause_event)
		CloseHandle(ui_pause_event);

	// kill the window thread ready event
	if (window_thread_ready_event)
		CloseHandle(window_thread_ready_event);
}

//============================================================
//  winrt_window_info::update
//  (main thread)
//============================================================

void winrt_window_info::update()
{
	int targetview, targetorient;
	render_layer_config targetlayerconfig;

	assert(GetCurrentThreadId() == main_threadid);

	// see if the target has changed significantly in window mode
	targetview = m_target->view();
	targetorient = m_target->orientation();
	targetlayerconfig = m_target->layer_config();
	if (targetview != m_targetview || targetorient != m_targetorient || targetlayerconfig != m_targetlayerconfig)
	{
		m_targetview = targetview;
		m_targetorient = targetorient;
		m_targetlayerconfig = targetlayerconfig;

		// in window mode, reminimize/maximize
		/*if (m_isminimized)
			SendMessage(m_hwnd, WM_USER_SET_MINSIZE, 0, 0);

		if (m_ismaximized)
			SendMessage(m_hwnd, WM_USER_SET_MAXSIZE, 0, 0);
		}*/
	}

	// if we're visible and running and not in the middle of a resize, draw
	if (m_window != nullptr && m_target != nullptr && m_renderer != nullptr)
	{
		bool got_lock = true;

		//mtlog_add("winwindow_video_window_update: try lock");

		// only block if we're throttled
		if (machine().video().throttled() || osd_ticks() - last_update_time > 250)
			m_render_lock.lock();
		else
			got_lock = m_render_lock.try_lock();

		// only render if we were able to get the lock
		if (got_lock)
		{
			render_primitive_list *primlist;

			//mtlog_add("winwindow_video_window_update: got lock");

			// don't hold the lock; we just used it to see if rendering was still happening
			m_render_lock.unlock();

			// ensure the target bounds are up-to-date, and then get the primitives
			primlist = m_renderer->get_primitives();

			// post a redraw request with the primitive list as a parameter
			last_update_time = osd_ticks();
			//mtlog_add("winwindow_video_window_update: PostMessage start");
			// SendMessage(m_hwnd, WM_USER_REDRAW, 0, (LPARAM)primlist);
			// mtlog_add("winwindow_video_window_update: PostMessage end");
		}
	}

	//mtlog_add("winwindow_video_window_update: end");
}

//============================================================
//  winwindow_video_window_create
//  (main thread)
//============================================================

void winrt_window_info::create(running_machine &machine, int index, osd_monitor_info *monitor, const osd_window_config *config)
{
	assert(GetCurrentThreadId() == main_threadid);

	// allocate a new window object
	winrt_window_info *window = global_alloc(winrt_window_info(machine));
	window->m_win_config = *config;
	window->m_monitor = monitor;
	window->m_index = index;

	// set main window
	if (index > 0)
	{
		for (auto w = win_window_list; w != nullptr; w = w->m_next)
		{
			if (w->m_index == 0)
			{
				window->m_main = w;
				break;
			}
		}
	}

	// add us to the list
	*last_window_ptr = window;
	last_window_ptr = &window->m_next;

	// load the layout
	window->m_target = machine.render().target_alloc();

	// set the specific view
	winrt_options &options = downcast<winrt_options &>(machine.options());

	const char *defview = options.view();
	window->set_starting_view(index, defview, options.view(index));

	// remember the current values in case they change
	window->m_targetview = window->m_target->view();
	window->m_targetorient = window->m_target->orientation();
	window->m_targetlayerconfig = window->m_target->layer_config();

	// TODO: Window Title
	//// make the window title
	//if (video_config.numscreens == 1)
	//	sprintf(window->m_title, "%s: %s [%s]", emulator_info::get_appname(), machine.system().description, machine.system().name);
	//else
	//	sprintf(window->m_title, "%s: %s [%s] - Screen %d", emulator_info::get_appname(), machine.system().description, machine.system().name, index);

	window->m_init_state = window->complete_create() ? -1 : 1;

	// handle error conditions
	if (window->m_init_state == -1)
		fatalerror("Unable to complete window creation\n");
}

//============================================================
//  winwindow_video_window_destroy
//  (main thread)
//============================================================

void winrt_window_info::destroy()
{
	winrt_window_info **prevptr;

	assert(GetCurrentThreadId() == main_threadid);

	// remove us from the list
	for (prevptr = &win_window_list; *prevptr != nullptr; prevptr = &(*prevptr)->m_next)
		if (*prevptr == this)
		{
			*prevptr = this->m_next;
			break;
		}

	// TODO: destroy the window
	if (m_window != nullptr)
	{
		//SendMessage(m_hwnd, WM_USER_SELF_TERMINATE, 0, 0);
	}

	// free the render target
	machine().render().target_free(m_target);
}

//============================================================
//  set_starting_view
//  (main thread)
//============================================================

void winrt_window_info::set_starting_view(int index, const char *defview, const char *view)
{
	int viewindex;

	assert(GetCurrentThreadId() == main_threadid);

	// choose non-auto over auto
	if (strcmp(view, "auto") == 0 && strcmp(defview, "auto") != 0)
		view = defview;

	// query the video system to help us pick a view
	viewindex = target()->configured_view(view, index, video_config.numscreens);

	// set the view
	target()->set_view(viewindex);
}

//============================================================
//  complete_create
//  (window thread)
//============================================================

int winrt_window_info::complete_create()
{
	int tempwidth, tempheight;
	HMENU menu = nullptr;

	assert(GetCurrentThreadId() == window_threadid);

	// get the monitor bounds
	osd_rect monitorbounds = m_monitor->position_size();

	// create the window menu if needed
	//if (downcast<winrt_options &>(machine().options()).menu())
	//{
	//	/*if (win_create_menu(machine(), &menu))
	//		return 1;*/
	//}

	//// create the window, but don't show it yet
	//m_hwnd = win_create_window_ex_utf8(
	//	m_fullscreen ? FULLSCREEN_STYLE_EX : WINDOW_STYLE_EX,
	//	"MAME",
	//	m_title,
	//	m_fullscreen ? FULLSCREEN_STYLE : WINDOW_STYLE,
	//	monitorbounds.left() + 20, monitorbounds.top() + 20,
	//	monitorbounds.left() + 100, monitorbounds.top() + 100,
	//	nullptr,//(win_window_list != nullptr) ? win_window_list->m_hwnd : nullptr,
	//	menu,
	//	GetModuleHandleUni(),
	//	nullptr);
	m_window = Windows::UI::Core::CoreWindow::GetForCurrentThread();

	if (m_window == nullptr)
		return 1;

	// set window #0 as the focus window for all windows, required for D3D & multimonitor
	//m_focus_hwnd = win_window_list->m_hwnd;

	// set a pointer back to us
	//SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);

	// skip the positioning stuff for -video none */
	if (video_config.mode == VIDEO_MODE_NONE)
		return 0;

	// adjust the window position to the initial width/height
	tempwidth = (m_win_config.width != 0) ? m_win_config.width : 640;
	tempheight = (m_win_config.height != 0) ? m_win_config.height : 480;
	/*SetWindowPos(m_hwnd, nullptr, monitorbounds.left() + 20, monitorbounds.top() + 20,
		monitorbounds.left() + tempwidth + wnd_extra_width(),
		monitorbounds.top() + tempheight + wnd_extra_height(),
		SWP_NOZORDER);*/

	// maximum or minimize as appropriate
	adjust_window_position_after_major_change();

	// show the window

	// finish off by trying to initialize DirectX; if we fail, ignore it
	if (m_renderer != nullptr)
	{
		delete m_renderer;
	}
	m_renderer = osd_renderer::make_for_type(video_config.mode, reinterpret_cast<osd_window *>(this));
	if (m_renderer->create())
		return 1;

	return 0;
}

//============================================================
//  adjust_window_position_after_major_change
//  (window thread)
//============================================================

void winrt_window_info::adjust_window_position_after_major_change()
{
	RECT oldrect;

	assert(GetCurrentThreadId() == window_threadid);

	Windows::Foundation::Rect bounds = m_window->Bounds;
	oldrect.top = bounds.Top;
	oldrect.bottom = bounds.Bottom;
	oldrect.left = bounds.Left;
	oldrect.right = bounds.Right;

	// get the current size
	osd_rect newrect = RECT_to_osd_rect(oldrect);

	// adjust the window size so the client area is what we want
	
	// constrain the existing size to the aspect ratio
	if (video_config.keepaspect)
		newrect = constrain_to_aspect_ratio(newrect, WMSZ_BOTTOMRIGHT);

	// adjust the position if different
	if (oldrect.left != newrect.left() || oldrect.top != newrect.top() ||
		oldrect.right != newrect.right() || oldrect.bottom != newrect.bottom())

		/*SetWindowPos(m_hwnd, m_fullscreen ? HWND_TOPMOST : HWND_TOP,
			newrect.left(), newrect.top(),
			newrect.width(), newrect.height(), 0);*/

	// take note of physical window size (used for lightgun coordinate calculation)
	if (this == win_window_list)
	{
		win_physical_width = newrect.width();
		win_physical_height = newrect.height();
		osd_printf_verbose("Physical width %d, height %d\n", win_physical_width, win_physical_height);
	}
}

//============================================================
//  winwindow_video_window_monitor
//  (window thread)
//============================================================

osd_monitor_info *winrt_window_info::winwindow_video_window_monitor(const osd_rect *proposed)
{
	osd_monitor_info *monitor;

	// in window mode, find the nearest
	if (proposed != nullptr)
	{
		RECT p;
		p.top = proposed->top();
		p.left = proposed->left();
		p.bottom = proposed->bottom();
		p.right = proposed->right();
		//monitor = winrt_monitor_info::monitor_from_handle(MonitorFromRect(&p, MONITOR_DEFAULTTONEAREST));
		monitor = m_monitor;
	}
	else
		monitor = m_monitor;
		//monitor = winrt_monitor_info::monitor_from_handle(MonitorFromWindow(m_hwnd, MONITOR_DEFAULTTONEAREST));

	// make sure we're up-to-date
	//monitor->refresh();
	return monitor;
}

static inline int better_mode(int width0, int height0, int width1, int height1, float desired_aspect)
{
	float aspect0 = (float)width0 / (float)height0;
	float aspect1 = (float)width1 / (float)height1;
	return (fabs(desired_aspect - aspect0) < fabs(desired_aspect - aspect1)) ? 0 : 1;
}

//============================================================
//  constrain_to_aspect_ratio
//  (window thread)
//============================================================

osd_rect winrt_window_info::constrain_to_aspect_ratio(const osd_rect &rect, int adjustment)
{
	INT32 extrawidth = wnd_extra_width();
	INT32 extraheight = wnd_extra_height();
	INT32 propwidth, propheight;
	INT32 minwidth, minheight;
	INT32 maxwidth, maxheight;
	INT32 adjwidth, adjheight;
	float desired_aspect = 1.0f;
	osd_dim window_dim = get_size();
	INT32 target_width = window_dim.width();
	INT32 target_height = window_dim.height();
	INT32 xscale = 1, yscale = 1;
	int newwidth, newheight;
	osd_monitor_info *monitor = winwindow_video_window_monitor(&rect);

	assert(GetCurrentThreadId() == window_threadid);

	// get the pixel aspect ratio for the target monitor
	float pixel_aspect = monitor->aspect();

	// determine the proposed width/height
	propwidth = rect.width() - extrawidth;
	propheight = rect.height() - extraheight;

	// based on which edge we are adjusting, take either the width, height, or both as gospel
	// and scale to fit using that as our parameter
	switch (adjustment)
	{
	case WMSZ_BOTTOM:
	case WMSZ_TOP:
		m_target->compute_visible_area(10000, propheight, pixel_aspect, m_target->orientation(), propwidth, propheight);
		break;

	case WMSZ_LEFT:
	case WMSZ_RIGHT:
		m_target->compute_visible_area(propwidth, 10000, pixel_aspect, m_target->orientation(), propwidth, propheight);
		break;

	default:
		m_target->compute_visible_area(propwidth, propheight, pixel_aspect, m_target->orientation(), propwidth, propheight);
		break;
	}

	// get the minimum width/height for the current layout
	m_target->compute_minimum_size(minwidth, minheight);

	// compute the appropriate visible area if we're trying to keepaspect
	if (video_config.keepaspect)
	{
		// make sure the monitor is up-to-date
		m_target->compute_visible_area(target_width, target_height, m_monitor->aspect(), m_target->orientation(), target_width, target_height);
		desired_aspect = (float)target_width / (float)target_height;
	}

	// non-integer scaling - often gives more pleasing results in full screen
	newwidth = target_width;
	newheight = target_height;
	if (!video_config.fullstretch)
	{
		// compute maximum integral scaling to fit the window
		xscale = (target_width + 2) / newwidth;
		yscale = (target_height + 2) / newheight;

		// try a little harder to keep the aspect ratio if desired
		if (video_config.keepaspect)
		{
			// if we could stretch more in the X direction, and that makes a better fit, bump the xscale
			while (newwidth * (xscale + 1) <= window_dim.width() &&
				better_mode(newwidth * xscale, newheight * yscale, newwidth * (xscale + 1), newheight * yscale, desired_aspect))
				xscale++;

			// if we could stretch more in the Y direction, and that makes a better fit, bump the yscale
			while (newheight * (yscale + 1) <= window_dim.height() &&
				better_mode(newwidth * xscale, newheight * yscale, newwidth * xscale, newheight * (yscale + 1), desired_aspect))
				yscale++;

			// now that we've maxed out, see if backing off the maximally stretched one makes a better fit
			if (window_dim.width() - newwidth * xscale < window_dim.height() - newheight * yscale)
			{
				while (better_mode(newwidth * xscale, newheight * yscale, newwidth * (xscale - 1), newheight * yscale, desired_aspect) && (xscale >= 0))
					xscale--;
			}
			else
			{
				while (better_mode(newwidth * xscale, newheight * yscale, newwidth * xscale, newheight * (yscale - 1), desired_aspect) && (yscale >= 0))
					yscale--;
			}
		}

		// ensure at least a scale factor of 1
		if (xscale <= 0) xscale = 1;
		if (yscale <= 0) yscale = 1;

		// apply the final scale
		newwidth *= xscale;
		newheight *= yscale;
	}

	// clamp against the absolute minimum
	propwidth = MAX(propwidth, MIN_WINDOW_DIM);
	propheight = MAX(propheight, MIN_WINDOW_DIM);

	// clamp against the minimum width and height
	propwidth = MAX(propwidth, minwidth);
	propheight = MAX(propheight, minheight);

	maxwidth = monitor->usuable_position_size().width() - extrawidth;
	maxheight = monitor->usuable_position_size().height() - extraheight;

	// further clamp to the maximum width/height in the window
	if (m_win_config.width != 0)
		maxwidth = MIN(maxwidth, m_win_config.width + extrawidth);
	if (m_win_config.height != 0)
		maxheight = MIN(maxheight, m_win_config.height + extraheight);

	// clamp to the maximum
	propwidth = MIN(propwidth, maxwidth);
	propheight = MIN(propheight, maxheight);

	// compute the adjustments we need to make
	adjwidth = (propwidth + extrawidth) - rect.width();
	adjheight = (propheight + extraheight) - rect.height();

	// based on which corner we're adjusting, constrain in different ways
	osd_rect ret(rect);

	switch (adjustment)
	{
	case WMSZ_BOTTOM:
	case WMSZ_BOTTOMRIGHT:
	case WMSZ_RIGHT:
		ret = rect.resize(rect.width() + adjwidth, rect.height() + adjheight);
		break;

	case WMSZ_BOTTOMLEFT:
		ret = rect.move_by(-adjwidth, 0).resize(rect.width() + adjwidth, rect.height() + adjheight);
		break;

	case WMSZ_LEFT:
	case WMSZ_TOPLEFT:
	case WMSZ_TOP:
		ret = rect.move_by(-adjwidth, -adjheight).resize(rect.width() + adjwidth, rect.height() + adjheight);
		break;

	case WMSZ_TOPRIGHT:
		ret = rect.move_by(0, -adjheight).resize(rect.width() + adjwidth, rect.height() + adjheight);
		break;
	}

	return ret;
}

//============================================================
//  winwindow_process_events
//  (main thread)
//============================================================

void winwindow_process_events(running_machine &machine, int ingame, bool nodispatch)
{
	assert(GetCurrentThreadId() == main_threadid);

	// TODO: Need to handle all the stuff that was in this method as events
	CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);

	// Review: Do we need this now?
	// update the cursor state after processing events
	// winwindow_update_cursor_state(machine);
}

void winrt_osd_interface::update_slider_list()
{
	for (winrt_window_info *window = win_window_list; window != nullptr; window = window->m_next)
	{
		if (window->m_renderer && window->m_renderer->sliders_dirty())
		{
			build_slider_list();
			return;
		}
	}
}

void winrt_osd_interface::build_slider_list()
{
	m_sliders = nullptr;
	slider_state *curr = m_sliders;
	for (winrt_window_info *info = win_window_list; info != nullptr; info = info->m_next)
	{
		slider_state *window_sliders = info->m_renderer->get_slider_list();
		if (window_sliders != nullptr)
		{
			if (m_sliders == nullptr)
			{
				m_sliders = curr = window_sliders;
			}
			else
			{
				while (curr->next != nullptr)
				{
					curr = curr->next;
				}
				curr->next = window_sliders;
			}
		}
	}
}