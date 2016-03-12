// license:BSD-3-Clause
// copyright-holders:Brad Hughes
//============================================================
//
//  winrtmain.cpp - Windows RT OSD
//
//============================================================

#include <windows.h>
#include <wrl\client.h>
#undef interface

#include "emu.h"
#include "osdepend.h"
#include "clifront.h"
#include "winrtmain.h"
#include "window.h"

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
		winrt_options options;
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

//**************************************************************************
//  OPTIONS
//**************************************************************************

// struct definitions
const options_entry winrt_options::s_option_entries[] =
{
	// performance options
	{ nullptr,                                        nullptr,    OPTION_HEADER,     "WINDOWS PERFORMANCE OPTIONS" },
	{ WINOPTION_PRIORITY "(-15-1)",                   "0",        OPTION_INTEGER,    "thread priority for the main game thread; range from -15 to 1" },
	{ WINOPTION_PROFILE,                              "0",        OPTION_INTEGER,    "enables profiling, specifying the stack depth to track" },

	// video options
	{ nullptr,                                        nullptr,    OPTION_HEADER,     "WINDOWS VIDEO OPTIONS" },
	{ WINOPTION_MENU,                                 "0",        OPTION_BOOLEAN,    "enables menu bar if available by UI implementation" },

	// DirectDraw-specific options
	{ nullptr,                                        nullptr,    OPTION_HEADER,     "DIRECTDRAW-SPECIFIC OPTIONS" },
	{ WINOPTION_HWSTRETCH ";hws",                     "1",        OPTION_BOOLEAN,    "enables hardware stretching" },

	// post-processing options
	{ nullptr,                                                  nullptr,             OPTION_HEADER,     "DIRECT3D POST-PROCESSING OPTIONS" },
	{ WINOPTION_HLSL_ENABLE";hlsl",                             "0",                 OPTION_BOOLEAN,    "enables HLSL post-processing (PS3.0 required)" },
	{ WINOPTION_HLSLPATH,                                       "hlsl",              OPTION_STRING,     "path to hlsl files" },
	{ WINOPTION_HLSL_PRESCALE_X,                                "0",                 OPTION_INTEGER,    "HLSL pre-scale override factor for X (0 for auto)" },
	{ WINOPTION_HLSL_PRESCALE_Y,                                "0",                 OPTION_INTEGER,    "HLSL pre-scale override factor for Y (0 for auto)" },
	{ WINOPTION_HLSL_WRITE,                                     nullptr,             OPTION_STRING,     "enables HLSL AVI writing (huge disk bandwidth suggested)" },
	{ WINOPTION_HLSL_SNAP_WIDTH,                                "2048",              OPTION_STRING,     "HLSL upscaled-snapshot width" },
	{ WINOPTION_HLSL_SNAP_HEIGHT,                               "1536",              OPTION_STRING,     "HLSL upscaled-snapshot height" },
	{ WINOPTION_SHADOW_MASK_TILE_MODE,                          "0",                 OPTION_INTEGER,    "shadow mask tile mode (0 for screen based, 1 for source based)" },
	{ WINOPTION_SHADOW_MASK_ALPHA";fs_shadwa(0.0-1.0)",         "0.0",               OPTION_FLOAT,      "shadow mask alpha-blend value (1.0 is fully blended, 0.0 is no mask)" },
	{ WINOPTION_SHADOW_MASK_TEXTURE";fs_shadwt(0.0-1.0)",       "shadow-mask.png",   OPTION_STRING,     "shadow mask texture name" },
	{ WINOPTION_SHADOW_MASK_COUNT_X";fs_shadww",                "6",                 OPTION_INTEGER,    "shadow mask tile width, in screen dimensions" },
	{ WINOPTION_SHADOW_MASK_COUNT_Y";fs_shadwh",                "4",                 OPTION_INTEGER,    "shadow mask tile height, in screen dimensions" },
	{ WINOPTION_SHADOW_MASK_USIZE";fs_shadwu(0.0-1.0)",         "0.1875",            OPTION_FLOAT,      "shadow mask texture width, in U/V dimensions" },
	{ WINOPTION_SHADOW_MASK_VSIZE";fs_shadwv(0.0-1.0)",         "0.25",              OPTION_FLOAT,      "shadow mask texture height, in U/V dimensions" },
	{ WINOPTION_SHADOW_MASK_UOFFSET";fs_shadwou(-1.0-1.0)",     "0.0",               OPTION_FLOAT,      "shadow mask texture offset, in U direction" },
	{ WINOPTION_SHADOW_MASK_VOFFSET";fs_shadwov(-1.0-1.0)",     "0.0",               OPTION_FLOAT,      "shadow mask texture offset, in V direction" },
	{ WINOPTION_CURVATURE";fs_curv(0.0-1.0)",                   "0.0",               OPTION_FLOAT,      "screen curvature amount" },
	{ WINOPTION_ROUND_CORNER";fs_rndc(0.0-1.0)",                "0.0",               OPTION_FLOAT,      "screen round corner amount" },
	{ WINOPTION_SMOOTH_BORDER";fs_smob(0.0-1.0)",               "0.0",               OPTION_FLOAT,      "screen smooth border amount" },
	{ WINOPTION_REFLECTION";fs_ref(0.0-1.0)",                   "0.0",               OPTION_FLOAT,      "screen reflection amount" },
	{ WINOPTION_VIGNETTING";fs_vig(0.0-1.0)",                   "0.0",               OPTION_FLOAT,      "image vignetting amount" },
	/* Beam-related values below this line*/
	{ WINOPTION_SCANLINE_AMOUNT";fs_scanam(0.0-4.0)",           "0.0",               OPTION_FLOAT,      "overall alpha scaling value for scanlines" },
	{ WINOPTION_SCANLINE_SCALE";fs_scansc(0.0-4.0)",            "1.0",               OPTION_FLOAT,      "overall height scaling value for scanlines" },
	{ WINOPTION_SCANLINE_HEIGHT";fs_scanh(0.0-4.0)",            "1.0",               OPTION_FLOAT,      "individual height scaling value for scanlines" },
	{ WINOPTION_SCANLINE_BRIGHT_SCALE";fs_scanbs(0.0-2.0)",     "1.0",               OPTION_FLOAT,      "overall brightness scaling value for scanlines (multiplicative)" },
	{ WINOPTION_SCANLINE_BRIGHT_OFFSET";fs_scanbo(0.0-1.0)",    "0.0",               OPTION_FLOAT,      "overall brightness offset value for scanlines (additive)" },
	{ WINOPTION_SCANLINE_JITTER";fs_scanjt(0.0-4.0)",           "0.0",               OPTION_FLOAT,      "overall interlace jitter scaling value for scanlines" },
	{ WINOPTION_HUM_BAR_ALPHA";fs_humba(0.0-1.0)",              "0.0",               OPTION_FLOAT,      "overall alpha scaling value for hum bar" },
	{ WINOPTION_DEFOCUS";fs_focus",                             "1.0,0.0",           OPTION_STRING,     "overall defocus value in screen-relative coords" },
	{ WINOPTION_CONVERGE_X";fs_convx",                          "0.25,0.00,-0.25",   OPTION_STRING,     "convergence in screen-relative X direction" },
	{ WINOPTION_CONVERGE_Y";fs_convy",                          "0.0,0.25,-0.25",    OPTION_STRING,     "convergence in screen-relative Y direction" },
	{ WINOPTION_RADIAL_CONVERGE_X";fs_rconvx",                  "0.0,0.0,0.0",       OPTION_STRING,     "radial convergence in screen-relative X direction" },
	{ WINOPTION_RADIAL_CONVERGE_Y";fs_rconvy",                  "0.0,0.0,0.0",       OPTION_STRING,     "radial convergence in screen-relative Y direction" },
	/* RGB colorspace convolution below this line */
	{ WINOPTION_RED_RATIO";fs_redratio",                        "1.0,0.0,0.0",       OPTION_STRING,     "red output signal generated by input signal" },
	{ WINOPTION_GRN_RATIO";fs_grnratio",                        "0.0,1.0,0.0",       OPTION_STRING,     "green output signal generated by input signal" },
	{ WINOPTION_BLU_RATIO";fs_bluratio",                        "0.0,0.0,1.0",       OPTION_STRING,     "blue output signal generated by input signal" },
	{ WINOPTION_SATURATION";fs_sat(0.0-4.0)",                   "1.4",               OPTION_FLOAT,      "saturation scaling value" },
	{ WINOPTION_OFFSET";fs_offset",                             "0.0,0.0,0.0",       OPTION_STRING,     "signal offset value (additive)" },
	{ WINOPTION_SCALE";fs_scale",                               "0.95,0.95,0.95",    OPTION_STRING,     "signal scaling value (multiplicative)" },
	{ WINOPTION_POWER";fs_power",                               "0.8,0.8,0.8",       OPTION_STRING,     "signal power value (exponential)" },
	{ WINOPTION_FLOOR";fs_floor",                               "0.05,0.05,0.05",    OPTION_STRING,     "signal floor level" },
	{ WINOPTION_PHOSPHOR";fs_phosphor",                         "0.4,0.4,0.4",       OPTION_STRING,     "phosphorescence decay rate (0.0 is instant, 1.0 is forever)" },
	/* NTSC simulation below this line */
	{ nullptr,                                                  nullptr,             OPTION_HEADER,     "NTSC POST-PROCESSING OPTIONS" },
	{ WINOPTION_YIQ_ENABLE";yiq",                               "0",                 OPTION_BOOLEAN,    "enables YIQ-space HLSL post-processing" },
	{ WINOPTION_YIQ_JITTER";yiqj",                              "0.0",               OPTION_FLOAT,      "Jitter for the NTSC signal processing" },
	{ WINOPTION_YIQ_CCVALUE";yiqcc",                            "3.57954545",        OPTION_FLOAT,      "Color Carrier frequency for NTSC signal processing" },
	{ WINOPTION_YIQ_AVALUE";yiqa",                              "0.5",               OPTION_FLOAT,      "A value for NTSC signal processing" },
	{ WINOPTION_YIQ_BVALUE";yiqb",                              "0.5",               OPTION_FLOAT,      "B value for NTSC signal processing" },
	{ WINOPTION_YIQ_OVALUE";yiqo",                              "0.0",               OPTION_FLOAT,      "Outgoing Color Carrier phase offset for NTSC signal processing" },
	{ WINOPTION_YIQ_PVALUE";yiqp",                              "1.0",               OPTION_FLOAT,      "Incoming Pixel Clock scaling value for NTSC signal processing" },
	{ WINOPTION_YIQ_NVALUE";yiqn",                              "1.0",               OPTION_FLOAT,      "Y filter notch width for NTSC signal processing" },
	{ WINOPTION_YIQ_YVALUE";yiqy",                              "6.0",               OPTION_FLOAT,      "Y filter cutoff frequency for NTSC signal processing" },
	{ WINOPTION_YIQ_IVALUE";yiqi",                              "1.2",               OPTION_FLOAT,      "I filter cutoff frequency for NTSC signal processing" },
	{ WINOPTION_YIQ_QVALUE";yiqq",                              "0.6",               OPTION_FLOAT,      "Q filter cutoff frequency for NTSC signal processing" },
	{ WINOPTION_YIQ_SCAN_TIME";yiqsc",                          "52.6",              OPTION_FLOAT,      "Horizontal scanline duration for NTSC signal processing (in usec)" },
	{ WINOPTION_YIQ_PHASE_COUNT";yiqp",                         "2",                 OPTION_INTEGER,    "Phase Count value for NTSC signal processing" },
	/* Vector simulation below this line */
	{ nullptr,                                                  nullptr,             OPTION_HEADER,     "VECTOR POST-PROCESSING OPTIONS" },
	{ WINOPTION_VECTOR_LENGTH_SCALE";veclength",                "0.5",               OPTION_FLOAT,      "How much length affects vector fade" },
	{ WINOPTION_VECTOR_LENGTH_RATIO";vecsize",                  "500.0",             OPTION_FLOAT,      "Vector fade length (4.0 - vectors fade the most at and above 4 pixels, etc.)" },
	/* Bloom below this line */
	{ nullptr,                                                  nullptr,             OPTION_HEADER,     "BLOOM POST-PROCESSING OPTIONS" },
	{ WINOPTION_BLOOM_BLEND_MODE,                               "0",                 OPTION_INTEGER,    "bloom blend mode (0 for addition, 1 for darken)" },
	{ WINOPTION_BLOOM_SCALE,                                    "0.25",              OPTION_FLOAT,      "Intensity factor for bloom" },
	{ WINOPTION_BLOOM_OVERDRIVE,                                "1.0,1.0,1.0",       OPTION_STRING,     "Overdrive factor for bloom" },
	{ WINOPTION_BLOOM_LEVEL0_WEIGHT,                            "1.0",               OPTION_FLOAT,      "Bloom level 0  (full-size target) weight" },
	{ WINOPTION_BLOOM_LEVEL1_WEIGHT,                            "0.64",              OPTION_FLOAT,      "Bloom level 1  (half-size target) weight" },
	{ WINOPTION_BLOOM_LEVEL2_WEIGHT,                            "0.32",              OPTION_FLOAT,      "Bloom level 2  (quarter-size target) weight" },
	{ WINOPTION_BLOOM_LEVEL3_WEIGHT,                            "0.16",              OPTION_FLOAT,      "Bloom level 3  (.) weight" },
	{ WINOPTION_BLOOM_LEVEL4_WEIGHT,                            "0.08",              OPTION_FLOAT,      "Bloom level 4  (.) weight" },
	{ WINOPTION_BLOOM_LEVEL5_WEIGHT,                            "0.04",              OPTION_FLOAT,      "Bloom level 5  (.) weight" },
	{ WINOPTION_BLOOM_LEVEL6_WEIGHT,                            "0.04",              OPTION_FLOAT,      "Bloom level 6  (.) weight" },
	{ WINOPTION_BLOOM_LEVEL7_WEIGHT,                            "0.02",              OPTION_FLOAT,      "Bloom level 7  (.) weight" },
	{ WINOPTION_BLOOM_LEVEL8_WEIGHT,                            "0.02",              OPTION_FLOAT,      "Bloom level 8  (.) weight" },
	{ WINOPTION_BLOOM_LEVEL9_WEIGHT,                            "0.01",              OPTION_FLOAT,      "Bloom level 9  (.) weight" },
	{ WINOPTION_BLOOM_LEVEL10_WEIGHT,                           "0.01",              OPTION_FLOAT,      "Bloom level 10 (1x1 target) weight" },

	// full screen options
	{ nullptr,                                        nullptr,    OPTION_HEADER,     "FULL SCREEN OPTIONS" },
	{ WINOPTION_TRIPLEBUFFER ";tb",                   "0",        OPTION_BOOLEAN,    "enables triple buffering" },
	{ WINOPTION_FULLSCREENBRIGHTNESS ";fsb(0.1-2.0)", "1.0",      OPTION_FLOAT,      "brightness value in full screen mode" },
	{ WINOPTION_FULLSCREENCONTRAST ";fsc(0.1-2.0)",   "1.0",      OPTION_FLOAT,      "contrast value in full screen mode" },
	{ WINOPTION_FULLSCREENGAMMA ";fsg(0.1-3.0)",      "1.0",      OPTION_FLOAT,      "gamma value in full screen mode" },

	// input options
	{ nullptr,                                        nullptr,    OPTION_HEADER,     "INPUT DEVICE OPTIONS" },
	{ WINOPTION_GLOBAL_INPUTS ";global_inputs",       "0",        OPTION_BOOLEAN,    "enables global inputs" },
	{ WINOPTION_DUAL_LIGHTGUN ";dual",                "0",        OPTION_BOOLEAN,    "enables dual lightgun input" },

	{ nullptr }
};


//============================================================
//  mini_osd_options
//============================================================

winrt_options::winrt_options()
	: osd_options()
{
	add_entries(winrt_options::s_option_entries);
}

//============================================================
//  constructor
//============================================================

winrt_osd_interface::winrt_osd_interface(winrt_options &options)
	: osd_common_t(options)
{
}

//============================================================
//  video_register
//============================================================

void winrt_osd_interface::video_register()
{
	video_options_add("bgfx", nullptr);
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

	winrt_options &options = downcast<winrt_options &>(machine.options());

	// initialize the subsystems
	osd_common_t::init_subsystems();

	// notify listeners of screen configuration
	/*for (winrt_window_info *info = win_window_list; info != nullptr; info = info->m_next)
	{
		machine.output().set_value(string_format("Orientation(%s)", info->m_monitor->devicename()).c_str(), info->m_targetorient);
	}*/
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

