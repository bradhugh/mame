// license:BSD-3-Clause
// copyright-holders:Aaron Giles, Brad Hughes
//============================================================
//
//  drawd3d11.h - Win32 Direct3D11 header
//
//============================================================

#ifndef __WIN_DRAWD3D11__
#define __WIN_DRAWD3D11__

#include "d3d11\d3d11comm.h"

struct D3DXVECTOR4;

namespace d3d11
{
	class texture_manager;
	struct vertex;
	class texture_info;
	class poly_info;

	typedef HRESULT(WINAPI* PFN_CREATEDXGIFACTORY2)(UINT, REFIID, LPVOID*);

	#define VERTEX_BUFFER_SIZE  (2048*4+4)

	typedef struct _VS_CONSTANT_BUFFER
	{
		float                TargetWidth;
		D3DXVECTOR3            Pad1;
		float                TargetHeight;
		D3DXVECTOR3            Pad2;
		float                PostPass;
		D3DXVECTOR3            Pad3;
	} VS_CONSTANT_BUFFER;

	static inline void ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
		{
		    throw emu_fatalerror("DirectX Call failed with result 0x%x", (unsigned int)hr);
		}
	}

	class renderer : public osd_renderer
	{
	public:
		//renderer() { }
		renderer(osd_window *window);
		virtual ~renderer();

		virtual int create() override;
		virtual render_primitive_list *get_primitives() override;
		virtual int draw(const int update) override;
		virtual void save() override;
		virtual void record() override;
		virtual void toggle_fsfx() override;
		virtual void destroy() override;

		int                                             initialize();

		int                                             device_create(HWND device_HWND);
		int                                             device_create_resources();
		int                                             create_size_dependent_resources();
		void                                            device_delete();
		void                                            device_delete_resources();

		int                                             device_verify_caps();
		int                                             device_test_cooperative();

		int                                             config_adapter_mode();
		void                                            pick_best_mode(_In_ UINT mode_count, _In_ const std::unique_ptr<DXGI_MODE_DESC[]> &modes);
		int                                             get_adapter_and_output_for_monitor();

		BOOL                                            update_window_size();

		int                                             pre_window_draw_check();
		void                                            begin_frame();
		void                                            end_frame();

		void                                            draw_line(const render_primitive *prim);
		void                                            draw_quad(const render_primitive *prim);
		void                                            batch_vector(const render_primitive *prim, float line_time);
		void                                            batch_vectors();

		vertex *                                        mesh_alloc(int numverts);

		void                                            process_primitives();
		void                                            primitive_flush_pending();

		void                                            set_texture(texture_info *texture);
		void                                            set_sampler(int filter, int wrap);
		void                                            set_modmode(DWORD modmode);
		void                                            set_blendmode(int blendmode);
		void                                            reset_render_states();

		void                                            set_fullscreen_gamma();
		int                                             get_output_modes(_Out_ UINT &mode_count, _Out_ std::unique_ptr<DXGI_MODE_DESC[]> &modes);
		HRESULT                                         create_dxgi_factory2_dynamic(UINT flags, REFIID riid, IDXGIFactory2 ** ppFactory);
		static float                                    refresh_as_float(const DXGI_RATIONAL &refresh);

#if defined(_DEBUG)
		inline bool SdkLayersAvailable();
#endif

        HRESULT                                         create_d3d11_device_dynamic(
                                                            IDXGIAdapter *              pAdapter,
                                                            D3D_DRIVER_TYPE             DriverType,
                                                            HMODULE                     Software,
                                                            UINT                        Flags,
                                                            const D3D_FEATURE_LEVEL *   pFeatureLevels,
                                                            UINT                        FeatureLevels,
                                                            UINT                        SDKVersion,
                                                            ID3D11Device **             ppDevice,
                                                            D3D_FEATURE_LEVEL*          pFeatureLevel,
                                                            ID3D11DeviceContext **      ppImmediateContext
                                                            );

		// Setters / getters
		int                                             get_adapter_index() { return m_adapter_index; }
		int                                             get_width() { return m_width; }
		vec2f                                           get_dims() { return vec2f(m_width, m_height); }
		int                                             get_height() { return m_height; }
		int                                             get_refresh() { return (int)refresh_as_float(m_currentmode.RefreshRate); }

		Microsoft::WRL::ComPtr<ID3D11Device1>           get_device() { return m_device; }
		DXGI_SWAP_CHAIN_DESC1*                          get_presentation() { return &m_presentation; }

		Microsoft::WRL::ComPtr<ID3D11Buffer>            get_vertex_buffer() { return m_vertexbuf; }
		vertex *                                        get_locked_buffer() { return m_lockedbuf; }
		VOID **                                         get_locked_buffer_ptr() { return (VOID **)&m_lockedbuf; }
		void                                            set_locked_buffer(vertex *lockedbuf) { m_lockedbuf = lockedbuf; }

		void                                            set_restarting(bool restarting) { m_restarting = restarting; }

		DXGI_FORMAT                                     get_screen_format() { return m_currentmode.Format; }
		DXGI_FORMAT                                     get_pixel_format() { return m_pixformat; }
		DXGI_MODE_DESC                                  get_origmode() { return m_origmode; }

		UINT32                                          get_last_texture_flags() { return m_last_texture_flags; }

		texture_manager *                               get_texture_manager() { return m_texture_manager; }
        D3D_FEATURE_LEVEL                               get_feature_level() { return m_d3dFeatureLevel; }
		Microsoft::WRL::ComPtr<ID3D11DeviceContext1>    get_context() { return m_device_context; }

	private:
		int                                             m_adapter_index;                    // ordinal adapter number
		Microsoft::WRL::ComPtr<IDXGIAdapter1>           m_adapter;                          // The adapter
		int                                             m_output_index;                     // ordinal output number
		Microsoft::WRL::ComPtr<IDXGIOutput>             m_output;                           // The adapter output (monitor)
		int                                             m_width;                            // current window width
		int                                             m_height;                           // current window height

		Microsoft::WRL::ComPtr<ID3D11Device1>           m_device;                           // pointer to the Direct3DDevice object
		Microsoft::WRL::ComPtr<ID3D11DeviceContext1>    m_device_context;
		DXGI_SWAP_CHAIN_DESC1                           m_presentation;                     // The swap chain descriptor
		DXGI_MODE_DESC                                  m_origmode;                         // The original display mode
		DXGI_MODE_DESC                                  m_currentmode;                      // The current display mode
		DXGI_FORMAT                                     m_pixformat;                        // pixel format we are using

		Microsoft::WRL::ComPtr<ID3D11Buffer>            m_vertexbuf;                        // pointer to the vertex buffer object
		vertex *                                        m_lockedbuf;                        // pointer to the locked vertex buffer
		int                                             m_numverts;                         // number of accumulated vertices

		vertex *                                        m_vectorbatch;                      // pointer to the vector batch buffer
		int                                             m_batchindex;                       // current index into the vector batch
				                                        
		poly_info                                       m_poly[VERTEX_BUFFER_SIZE / 3];     // array to hold polygons as they are created
		int                                             m_numpolys;                         // number of accumulated polygons
				                                        
		bool                                            m_restarting;                       // if we're restarting

		texture_info *                                  m_last_texture;                     // previous texture
		UINT32                                          m_last_texture_flags;               // previous texture flags
		int                                             m_last_blendenable;                 // previous blendmode
		int                                             m_last_blendop;                     // previous blendmode
		int                                             m_last_blendsrc;                    // previous blendmode
		int                                             m_last_blenddst;                    // previous blendmode
		int                                             m_last_filter;                      // previous texture filter
		int                                             m_last_wrap;                        // previous wrap state
		DWORD                                           m_last_modmode;                      // previous texture modulation

		texture_manager *                               m_texture_manager;                  // texture manager

		int                                             m_line_count;

		VS_CONSTANT_BUFFER                              m_shader_settings;                  // Shader settings
		Microsoft::WRL::ComPtr<ID3D11VertexShader>      m_vertex_shader;                    // The vertex shader object
		Microsoft::WRL::ComPtr<ID3D11PixelShader>       m_pixel_shader;                     // The pixel shader object
		Microsoft::WRL::ComPtr<ID3D11Buffer>            m_shader_settings_buffer;           // Shader settings buffer
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView>  m_render_target_view;               // The render target view
		Microsoft::WRL::ComPtr<ID3D11SamplerState>      m_sampler_state;
		Microsoft::WRL::ComPtr<IDXGIFactory2>           m_dxgiFactory;
		D3D_FEATURE_LEVEL                               m_d3dFeatureLevel;
		int                                             m_rendertarget_width;               // the render target width (adjusted for orientation and DPI)
		int                                             m_rendertarget_height;              // the render target height (adjusted for orientation and DPI)
		BOOL                                            m_fullscreen;                       // the full screen mode

		DXGI_MODE_ROTATION                              m_rotation;                         // rotation of the display

		Microsoft::WRL::ComPtr<ID3D11InputLayout>       m_layout;                           // The input layout object
		Microsoft::WRL::ComPtr<IDXGISwapChain1>         m_swap_chain;                       // The swap chain

		HMODULE                                         m_d3d_module;
		HMODULE                                         m_dxgi_module;
		PFN_CREATEDXGIFACTORY2                          m_pfnCreateDXGIFactory2;
		PFN_D3D11_CREATE_DEVICE                         m_pfnD3D11CreateDevice;
	};

}

#endif