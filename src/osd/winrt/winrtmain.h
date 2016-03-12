// license:BSD-3-Clause
// copyright-holders:Brad Hughes
//============================================================
//
//  winrtmain.h - Windows RT OSD
//
//============================================================

#pragma once

#include "options.h"
#include "osdepend.h"
#include "modules/lib/osdobj_common.h"

#include <memory>

// performance options
#define WINOPTION_PRIORITY              "priority"
#define WINOPTION_PROFILE               "profile"

// video options
#define WINOPTION_MENU                  "menu"

// DirectDraw-specific options
#define WINOPTION_HWSTRETCH             "hwstretch"

// core post-processing options
#define WINOPTION_HLSL_ENABLE               "hlsl_enable"
#define WINOPTION_HLSLPATH                  "hlslpath"
#define WINOPTION_HLSL_PRESCALE_X           "hlsl_prescale_x"
#define WINOPTION_HLSL_PRESCALE_Y           "hlsl_prescale_y"
#define WINOPTION_HLSL_WRITE                "hlsl_write"
#define WINOPTION_HLSL_SNAP_WIDTH           "hlsl_snap_width"
#define WINOPTION_HLSL_SNAP_HEIGHT          "hlsl_snap_height"
#define WINOPTION_SHADOW_MASK_TILE_MODE     "shadow_mask_tile_mode"
#define WINOPTION_SHADOW_MASK_ALPHA         "shadow_mask_alpha"
#define WINOPTION_SHADOW_MASK_TEXTURE       "shadow_mask_texture"
#define WINOPTION_SHADOW_MASK_COUNT_X       "shadow_mask_x_count"
#define WINOPTION_SHADOW_MASK_COUNT_Y       "shadow_mask_y_count"
#define WINOPTION_SHADOW_MASK_USIZE         "shadow_mask_usize"
#define WINOPTION_SHADOW_MASK_VSIZE         "shadow_mask_vsize"
#define WINOPTION_SHADOW_MASK_UOFFSET       "shadow_mask_uoffset"
#define WINOPTION_SHADOW_MASK_VOFFSET       "shadow_mask_voffset"
#define WINOPTION_REFLECTION                "reflection"
#define WINOPTION_CURVATURE                 "curvature"
#define WINOPTION_ROUND_CORNER              "round_corner"
#define WINOPTION_SMOOTH_BORDER             "smooth_border"
#define WINOPTION_VIGNETTING                "vignetting"
#define WINOPTION_SCANLINE_AMOUNT           "scanline_alpha"
#define WINOPTION_SCANLINE_SCALE            "scanline_size"
#define WINOPTION_SCANLINE_HEIGHT           "scanline_height"
#define WINOPTION_SCANLINE_BRIGHT_SCALE     "scanline_bright_scale"
#define WINOPTION_SCANLINE_BRIGHT_OFFSET    "scanline_bright_offset"
#define WINOPTION_SCANLINE_JITTER           "scanline_jitter"
#define WINOPTION_HUM_BAR_ALPHA             "hum_bar_alpha"
#define WINOPTION_DEFOCUS                   "defocus"
#define WINOPTION_CONVERGE_X                "converge_x"
#define WINOPTION_CONVERGE_Y                "converge_y"
#define WINOPTION_RADIAL_CONVERGE_X         "radial_converge_x"
#define WINOPTION_RADIAL_CONVERGE_Y         "radial_converge_y"
#define WINOPTION_RED_RATIO                 "red_ratio"
#define WINOPTION_GRN_RATIO                 "grn_ratio"
#define WINOPTION_BLU_RATIO                 "blu_ratio"
#define WINOPTION_OFFSET                    "offset"
#define WINOPTION_SCALE                     "scale"
#define WINOPTION_POWER                     "power"
#define WINOPTION_FLOOR                     "floor"
#define WINOPTION_PHOSPHOR                  "phosphor_life"
#define WINOPTION_SATURATION                "saturation"
#define WINOPTION_YIQ_ENABLE                "yiq_enable"
#define WINOPTION_YIQ_JITTER                "yiq_jitter"
#define WINOPTION_YIQ_CCVALUE               "yiq_cc"
#define WINOPTION_YIQ_AVALUE                "yiq_a"
#define WINOPTION_YIQ_BVALUE                "yiq_b"
#define WINOPTION_YIQ_OVALUE                "yiq_o"
#define WINOPTION_YIQ_PVALUE                "yiq_p"
#define WINOPTION_YIQ_NVALUE                "yiq_n"
#define WINOPTION_YIQ_YVALUE                "yiq_y"
#define WINOPTION_YIQ_IVALUE                "yiq_i"
#define WINOPTION_YIQ_QVALUE                "yiq_q"
#define WINOPTION_YIQ_SCAN_TIME             "yiq_scan_time"
#define WINOPTION_YIQ_PHASE_COUNT           "yiq_phase_count"
#define WINOPTION_VECTOR_LENGTH_SCALE       "vector_length_scale"
#define WINOPTION_VECTOR_LENGTH_RATIO       "vector_length_ratio"
#define WINOPTION_VECTOR_TIME_PERIOD        "vector_time_period"
#define WINOPTION_BLOOM_BLEND_MODE          "bloom_blend_mode"
#define WINOPTION_BLOOM_SCALE               "bloom_scale"
#define WINOPTION_BLOOM_OVERDRIVE           "bloom_overdrive"
#define WINOPTION_BLOOM_LEVEL0_WEIGHT       "bloom_lvl0_weight"
#define WINOPTION_BLOOM_LEVEL1_WEIGHT       "bloom_lvl1_weight"
#define WINOPTION_BLOOM_LEVEL2_WEIGHT       "bloom_lvl2_weight"
#define WINOPTION_BLOOM_LEVEL3_WEIGHT       "bloom_lvl3_weight"
#define WINOPTION_BLOOM_LEVEL4_WEIGHT       "bloom_lvl4_weight"
#define WINOPTION_BLOOM_LEVEL5_WEIGHT       "bloom_lvl5_weight"
#define WINOPTION_BLOOM_LEVEL6_WEIGHT       "bloom_lvl6_weight"
#define WINOPTION_BLOOM_LEVEL7_WEIGHT       "bloom_lvl7_weight"
#define WINOPTION_BLOOM_LEVEL8_WEIGHT       "bloom_lvl8_weight"
#define WINOPTION_BLOOM_LEVEL9_WEIGHT       "bloom_lvl9_weight"
#define WINOPTION_BLOOM_LEVEL10_WEIGHT      "bloom_lvl10_weight"

// full screen options
#define WINOPTION_TRIPLEBUFFER          "triplebuffer"
#define WINOPTION_FULLSCREENBRIGHTNESS  "full_screen_brightness"
#define WINOPTION_FULLSCREENCONTRAST    "full_screen_contrast"
#define WINOPTION_FULLSCREENGAMMA       "full_screen_gamma"

// input options
#define WINOPTION_GLOBAL_INPUTS         "global_inputs"
#define WINOPTION_DUAL_LIGHTGUN         "dual_lightgun"


class winrt_options : public osd_options
{
public:
	// construction/destruction
	winrt_options();
private:
	static const options_entry s_option_entries[];
};

//============================================================
//  TYPE DEFINITIONS
//============================================================

class winrt_osd_interface : public osd_common_t
{
public:
	// construction/destruction
	winrt_osd_interface(winrt_options &options);
	virtual ~winrt_osd_interface();

	// general overridables
	virtual void init(running_machine &machine);
	virtual void update(bool skip_redraw);

	// video overridables
	virtual slider_state *get_slider_list() override;

	void video_register() override;
	bool window_init() override;
	void window_exit() override;
	bool video_init() override;

	void extract_video_config();

	// winrt osd specific
	void poll_input(running_machine &machine);

	winrt_options &options() { return m_options; }

private:
	void build_slider_list();
	void update_slider_list();

	winrt_options &     m_options;
	slider_state *      m_sliders;
};

namespace MameWinRT
{
	// Main entry point for our app. Connects the app with the Windows shell and handles application lifecycle events.
	ref class App sealed : public Windows::ApplicationModel::Core::IFrameworkView
	{
	public:
		App();

		// IFrameworkView Methods.
		virtual void Initialize(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView);
		virtual void SetWindow(Windows::UI::Core::CoreWindow^ window);
		virtual void Load(Platform::String^ entryPoint);
		virtual void Run();
		virtual void Uninitialize();

	protected:
		// Application lifecycle event handlers.
		void OnActivated(Windows::ApplicationModel::Core::CoreApplicationView^ applicationView, Windows::ApplicationModel::Activation::IActivatedEventArgs^ args);
		void OnSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ args);
		void OnResuming(Platform::Object^ sender, Platform::Object^ args);

		// Window event handlers.
		void OnWindowSizeChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::WindowSizeChangedEventArgs^ args);
		void OnVisibilityChanged(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::VisibilityChangedEventArgs^ args);
		void OnWindowClosed(Windows::UI::Core::CoreWindow^ sender, Windows::UI::Core::CoreWindowEventArgs^ args);

		// DisplayInformation event handlers.
		void OnDpiChanged(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);
		void OnOrientationChanged(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);
		void OnDisplayContentsInvalidated(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);

	private:
		bool m_windowClosed;
		bool m_windowVisible;
	};
}

ref class Direct3DApplicationSource sealed : Windows::ApplicationModel::Core::IFrameworkViewSource
{
public:
	virtual Windows::ApplicationModel::Core::IFrameworkView^ CreateView();
};
