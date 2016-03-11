// license:BSD-3-Clause
// copyright-holders:Brad Hughes
//============================================================
//
//  winrtmain.cpp - Windows RT OSD
//
//============================================================

#include <windows.h>
#undef interface

#include "emu.h"
#include "osdepend.h"
#include "clifront.h"
#include "winrtmain.h"

#include <memory>
#include <vector>

using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::UI::Core;
using namespace Windows::UI::Input;
using namespace Windows::System;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;

using namespace MameWinRT;

Platform::Array<Platform::String^>^ g_command_args;

// The main function is only used to initialize our IFrameworkView class.
[Platform::MTAThread]
int main(Platform::Array<Platform::String^>^ args)
{
	g_command_args = args;
	auto direct3DApplicationSource = ref new Direct3DApplicationSource();
	CoreApplication::Run(direct3DApplicationSource);
	return 0;
}

IFrameworkView^ Direct3DApplicationSource::CreateView()
{
	return ref new App();
}

App::App() :
	m_windowClosed(false),
	m_windowVisible(true)
{
}

// The first method called when the IFrameworkView is being created.
void App::Initialize(CoreApplicationView^ applicationView)
{
	// Register event handlers for app lifecycle. This example includes Activated, so that we
	// can make the CoreWindow active and start rendering on the window.
	applicationView->Activated +=
		ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &App::OnActivated);

	CoreApplication::Suspending +=
		ref new EventHandler<SuspendingEventArgs^>(this, &App::OnSuspending);

	CoreApplication::Resuming +=
		ref new EventHandler<Platform::Object^>(this, &App::OnResuming);
}

// Called when the CoreWindow object is created (or re-created).
void App::SetWindow(CoreWindow^ window)
{
	window->SizeChanged +=
		ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(this, &App::OnWindowSizeChanged);

	window->VisibilityChanged +=
		ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &App::OnVisibilityChanged);

	window->Closed +=
		ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(this, &App::OnWindowClosed);

	DisplayInformation^ currentDisplayInformation = DisplayInformation::GetForCurrentView();

	currentDisplayInformation->DpiChanged +=
		ref new TypedEventHandler<DisplayInformation^, Object^>(this, &App::OnDpiChanged);

	currentDisplayInformation->OrientationChanged +=
		ref new TypedEventHandler<DisplayInformation^, Object^>(this, &App::OnOrientationChanged);

	DisplayInformation::DisplayContentsInvalidated +=
		ref new TypedEventHandler<DisplayInformation^, Object^>(this, &App::OnDisplayContentsInvalidated);
}

// Initializes scene resources, or loads a previously saved app state.
void App::Load(Platform::String^ entryPoint)
{
	if (m_main == nullptr)
	{
		m_main = std::unique_ptr<MameMain>(new MameMain());
	}
}

// This method is called after the window becomes active.
void App::Run()
{
	// parse config and cmdline options
	DWORD result;
	{
		winrt_osd_options options;
		winrt_osd_interface osd(options);
		// if we're a GUI app, out errors to message boxes
		// Initialize this after the osd interface so that we are first in the
		// output order

		/*std::vector<std::unique_ptr<char>> arglist(g_command_args->Length);
		auto args = std::make_unique<char*[]>(g_command_args->Length);
		for (int i = 0; i < g_command_args->Length; i++)
		{
			Platform::String^ arg = g_command_args->Data[i];
			unsigned int arglen = arg->Length();

			auto utf8param = std::make_unique<char>(arglen + 1);
			if (!WideCharToMultiByte(CP_UTF8, 0, arg->Data(), arglen, utf8param.get(), arglen + 1, nullptr, nullptr))
			{
				throw new std::exception("WideCharToMultiByte failed!");
			}

			arglist.push_back(std::move(utf8param));
			args.get()[i] = arglist.back().get();
		}*/

		const char * default_exe_path = "C:\\Fake\\WindowsStoreRoot\\Mame.exe";
		char exe_path[MAX_PATH];
		char* fake_args[1] = { exe_path };

		strncpy(exe_path, default_exe_path, MAX_PATH);
		
		osd.register_options();
		cli_frontend frontend(options, osd);
		result = frontend.execute(1, fake_args);
	}
}

// Required for IFrameworkView.
// Terminate events do not cause Uninitialize to be called. It will be called if your IFrameworkView
// class is torn down while the app is in the foreground.
void App::Uninitialize()
{
}

// Application lifecycle event handlers.

void App::OnActivated(CoreApplicationView^ applicationView, IActivatedEventArgs^ args)
{
	// Run() won't start until the CoreWindow is activated.
	CoreWindow::GetForCurrentThread()->Activate();
}

void App::OnSuspending(Platform::Object^ sender, SuspendingEventArgs^ args)
{
	// Save app state asynchronously after requesting a deferral. Holding a deferral
	// indicates that the application is busy performing suspending operations. Be
	// aware that a deferral may not be held indefinitely. After about five seconds,
	// the app will be forced to exit.
	SuspendingDeferral^ deferral = args->SuspendingOperation->GetDeferral();

	//create_task([this, deferral]()
	//{
	//	//m_deviceResources->Trim();

	//	// Insert your code here.

	//	deferral->Complete();
	//});
}

void App::OnResuming(Platform::Object^ sender, Platform::Object^ args)
{
	// Restore any data or state that was unloaded on suspend. By default, data
	// and state are persisted when resuming from suspend. Note that this event
	// does not occur if the app was previously terminated.

	// Insert your code here.
}

// Window event handlers.

void App::OnWindowSizeChanged(CoreWindow^ sender, WindowSizeChangedEventArgs^ args)
{
}

void App::OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args)
{
	m_windowVisible = args->Visible;
}

void App::OnWindowClosed(CoreWindow^ sender, CoreWindowEventArgs^ args)
{
	m_windowClosed = true;
}

// DisplayInformation event handlers.
void App::OnDpiChanged(DisplayInformation^ sender, Object^ args)
{
}

void App::OnOrientationChanged(DisplayInformation^ sender, Object^ args)
{
}

void App::OnDisplayContentsInvalidated(DisplayInformation^ sender, Object^ args)
{
}

// Loads and initializes application assets when the application is loaded.
MameMain::MameMain()
{
}

MameMain::~MameMain()
{
}

// Updates application state when the window size changes (e.g. device orientation change)
void MameMain::CreateWindowSizeDependentResources()
{
}

// Updates the application state once per frame.
void MameMain::Update()
{
}

// Renders the current frame according to the current application state.
// Returns true if the frame was rendered and is ready to be displayed.
bool MameMain::Render()
{
	return true;
}

// Notifies renderers that device resources need to be released.
void MameMain::OnDeviceLost()
{
	/*m_sceneRenderer->ReleaseDeviceDependentResources();
	m_fpsTextRenderer->ReleaseDeviceDependentResources();*/
}

// Notifies renderers that device resources may now be recreated.
void MameMain::OnDeviceRestored()
{
	/*m_sceneRenderer->CreateDeviceDependentResources();
	m_fpsTextRenderer->CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();*/
}

//============================================================
//  CONSTANTS
//============================================================

// we fake a keyboard with the following keys
enum
{
	KEY_ESCAPE,
	KEY_P1_START,
	KEY_BUTTON_1,
	KEY_BUTTON_2,
	KEY_BUTTON_3,
	KEY_JOYSTICK_U,
	KEY_JOYSTICK_D,
	KEY_JOYSTICK_L,
	KEY_JOYSTICK_R,
	KEY_TOTAL
};

//============================================================
//  GLOBALS
//============================================================

// a single rendering target
static render_target *our_target;

// a single input device
static input_device *keyboard_device;

// the state of each key
static UINT8 keyboard_state[KEY_TOTAL];


//============================================================
//  mini_osd_options
//============================================================

winrt_osd_options::winrt_osd_options()
	: osd_options()
{
	//add_entries(s_option_entries);
}

//============================================================
//  constructor
//============================================================

winrt_osd_interface::winrt_osd_interface(winrt_osd_options &options)
	: osd_common_t(options)
{
}


//============================================================
//  destructor
//============================================================

winrt_osd_interface::~winrt_osd_interface()
{
}


//============================================================
//  init
//============================================================

void winrt_osd_interface::init(running_machine &machine)
{
	// call our parent
	osd_common_t::init(machine);

	// initialize the video system by allocating a rendering target
	our_target = machine.render().target_alloc();

	// nothing yet to do to initialize sound, since we don't have any
	// sound updates are handled by update_audio_stream() below

	// hook up the debugger log
	//  add_logerror_callback(machine, output_oslog);
}


//============================================================
//  osd_update
//============================================================

void winrt_osd_interface::update(bool skip_redraw)
{
	// get the minimum width/height for the current layout
	int minwidth, minheight;
	our_target->compute_minimum_size(minwidth, minheight);

	// make that the size of our target
	our_target->set_bounds(minwidth, minheight);

	// get the list of primitives for the target at the current size
	render_primitive_list &primlist = our_target->get_primitives();

	// lock them, and then render them
	primlist.acquire_lock();

	// do the drawing here
	primlist.release_lock();

	// after 5 seconds, exit
	if (machine().time() > attotime::from_seconds(5))
		machine().schedule_exit();
}

