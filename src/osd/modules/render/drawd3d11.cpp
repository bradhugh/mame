// license:BSD-3-Clause
// copyright-holders:Aaron Giles, Brad Hughes
//============================================================
//
//  drawd3d11.c - Win32 Direct3D 11 implementation
//
//============================================================

// Useful info:
//  Windows XP/2003 shipped with DirectX 8.1
//  Windows 2000 shipped with DirectX 7a
//  Windows 98SE shipped with DirectX 6.1a
//  Windows 98 shipped with DirectX 5
//  Windows NT shipped with DirectX 3.0a
//  Windows 95 shipped with DirectX 2

#include "modules\render\d3d11\d3d11winsdk.h"

// Shaders
#include "modules\render\d3d11\PixelShader.h"
#include "modules\render\d3d11\SimpleVertexShader.h"

#undef interface

// MAME headers
#include "emu.h"
#include "render.h"

#include "rendutil.h"
#include "emuopts.h"
#include "aviio.h"

// MAMEOS headers
#include "modules/render/d3d/d3dintf.h"
#include "winmain.h"
#include "window.h"
#include "modules/render/d3d/d3dcomm.h"
#include "winfile.h"

#include "drawd3d11.h"

//============================================================
//  DEBUGGING
//============================================================

extern void mtlog_add(const char *event);

//============================================================
//  drawd3d_window_init
//============================================================

int d3d11::renderer::create()
{
    if (!initialize())
    {
        destroy();
        osd_printf_error("Unable to initialize Direct3D 11.\n");
        return 1;
    }

    return 0;
}

//============================================================
//  drawd3d11_exit
//============================================================

static void drawd3d11_exit(void)
{
}

void d3d11::renderer::toggle_fsfx()
{
    set_restarting(true);
}

void d3d11::renderer::record()
{
}

void d3d11::renderer::save()
{
}

//============================================================
//  drawd3d_window_destroy
//============================================================

void d3d11::renderer::destroy()
{
}

//============================================================
//  drawd3d_window_get_primitives
//============================================================

render_primitive_list *d3d11::renderer::get_primitives()
{
    RECT client;

    GetClientRectExceptMenu(window().m_hwnd, &client, window().fullscreen());
    if (rect_width(&client) > 0 && rect_height(&client) > 0)
    {
        window().target()->set_bounds(rect_width(&client), rect_height(&client), window().aspect());

        window().target()->set_max_update_rate(refresh_as_float(m_currentmode.RefreshRate));
    }

    return &window().target()->get_primitives();
}

//============================================================
//  drawd3d11_create
//============================================================

static osd_renderer *drawd3d11_create(osd_window *window)
{
	return global_alloc(d3d11::renderer(window));
}

int drawd3d11_init(running_machine &machine, osd_draw_callbacks *callbacks)
{
	// fill in the callbacks
	memset(callbacks, 0, sizeof(*callbacks));
	callbacks->exit = drawd3d11_exit;
	callbacks->create = drawd3d11_create;

    osd_printf_verbose("Direct3D: Using Direct3D 11\n");
	return 0;
}

//============================================================
//  drawd3d_window_draw
//============================================================

int d3d11::renderer::draw(const int update)
{
    int check = pre_window_draw_check();
    if (check >= 0)
        return check;

    begin_frame();
    process_primitives();
    end_frame();

    return 0;
}

namespace d3d11
{
	using namespace std;
	using namespace Microsoft::WRL;

    void renderer::set_texture(texture_info *texture)
    {
        if (texture != m_last_texture)
        {
            m_last_texture = texture;
            m_last_texture_flags = texture == NULL ? 0 : texture->get_flags();

            if (texture == NULL)
            {
                texture = m_texture_manager->get_default_texture();
            }

            // Create the Shader Resource View if needed
            if (nullptr == texture->get_surface())
            {
                D3D11_TEXTURE2D_DESC texDesc;
                texture->get_finaltex()->GetDesc(&texDesc);

                D3D11_SHADER_RESOURCE_VIEW_DESC shaderResDesc;
                memset(&shaderResDesc, 0, sizeof(shaderResDesc));
                shaderResDesc.Format = texDesc.Format;
                shaderResDesc.ViewDimension = D3D_SRV_DIMENSION_TEXTURE2D;
                shaderResDesc.Texture2D.MipLevels = texDesc.MipLevels;

                // Create the Shader Resource View
                ComPtr<ID3D11ShaderResourceView> surface;
                HRESULT result = m_device->CreateShaderResourceView(
                    texture->get_finaltex().Get(),
                    &shaderResDesc,
                    surface.GetAddressOf());

                if (result != S_OK)
                {
                    osd_printf_verbose("Direct3D: Error %08X during device set_texture call\n", (int)result);
                    throw emu_fatalerror("Direct3D: Error %08X during device set_texture call\n", (int)result);
                }

                NAME_OBJECT(surface, "Shader Resource View");

                // Now set this on the texture
                texture->set_surface(surface);
            }

            m_device_context->PSSetShaderResources(0, 1, texture->get_surface().GetAddressOf());
        }
    }

    void renderer::set_sampler(int filter, int wrap)
    {
        HRESULT result;
        if (filter != m_last_filter || wrap != m_last_wrap)
        {
            D3D11_SAMPLER_DESC samplerDesc;
            memset(&samplerDesc, 0, sizeof(samplerDesc));
            samplerDesc.Filter = filter ? D3D11_FILTER_MIN_MAG_MIP_LINEAR : D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
            samplerDesc.AddressU = wrap ? D3D11_TEXTURE_ADDRESS_WRAP : D3D11_TEXTURE_ADDRESS_CLAMP;
            samplerDesc.AddressV = wrap ? D3D11_TEXTURE_ADDRESS_WRAP : D3D11_TEXTURE_ADDRESS_CLAMP;
            samplerDesc.AddressW = wrap ? D3D11_TEXTURE_ADDRESS_WRAP : D3D11_TEXTURE_ADDRESS_CLAMP;
            samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
            samplerDesc.MaxLOD = FLT_MAX; // float max value

                                          // Since we're using ComPtr, all previous samplers should auto-release
            result = m_device->CreateSamplerState(&samplerDesc, m_sampler_state.ReleaseAndGetAddressOf());
            NAME_OBJECT(m_sampler_state, "Sampler State");
            if (result != S_OK)
            {
                osd_printf_verbose("Direct3D: Error %08X during device CreateSamplerState call\n", (unsigned int)result);
            }

            m_device_context->PSSetSamplers(0, 1, m_sampler_state.GetAddressOf());

            // Set our "last" values
            m_last_filter = filter;
            m_last_wrap = wrap;
        }
    }

    void renderer::set_blendmode(int blendmode)
    {
        BOOL blendenable;
        D3D11_BLEND_OP blendop;
        D3D11_BLEND blendsrc;
        D3D11_BLEND blenddst;

        // choose the parameters
        switch (blendmode)
        {
        default:
        case BLENDMODE_NONE:            blendenable = FALSE;    blendop = D3D11_BLEND_OP_ADD;   blendsrc = D3D11_BLEND_SRC_ALPHA;   blenddst = D3D11_BLEND_INV_SRC_ALPHA;    break;
        case BLENDMODE_ALPHA:           blendenable = TRUE;     blendop = D3D11_BLEND_OP_ADD;   blendsrc = D3D11_BLEND_SRC_ALPHA;   blenddst = D3D11_BLEND_INV_SRC_ALPHA;    break;
        case BLENDMODE_RGB_MULTIPLY:    blendenable = TRUE;     blendop = D3D11_BLEND_OP_ADD;   blendsrc = D3D11_BLEND_DEST_COLOR;  blenddst = D3D11_BLEND_ZERO;			 break;
        case BLENDMODE_ADD:             blendenable = TRUE;     blendop = D3D11_BLEND_OP_ADD;   blendsrc = D3D11_BLEND_SRC_ALPHA;   blenddst = D3D11_BLEND_ONE;              break;
        }

        // adjust the bits that changed
        if (blendenable != m_last_blendenable
            || blendop != m_last_blendop
            || blendsrc != m_last_blendsrc
            || blenddst != m_last_blenddst)
        {
            // Create our descriptor
            D3D11_BLEND_DESC desc = { 0 };

            // Configure our properties on render target 0
            desc.RenderTarget[0].BlendEnable = blendenable;
            desc.RenderTarget[0].BlendOp = blendop;
            desc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
            desc.RenderTarget[0].SrcBlend = blendsrc;
            desc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
            desc.RenderTarget[0].DestBlend = blenddst;
            desc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
            desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

            // Create and set the updated blend state
            ComPtr<ID3D11BlendState> blendState;
            m_device->CreateBlendState(&desc, &blendState);
            NAME_OBJECT(blendState, "Blend State");
            m_device_context->OMSetBlendState(blendState.Get(), nullptr, 0xFFFFFFFF);

            // Update our *last* states
            m_last_blendenable = blendenable;
            m_last_blendop = blendop;
            m_last_blendsrc = blendsrc;
            m_last_blenddst = blenddst;
        }
    }

    void renderer::reset_render_states()
    {
        // this ensures subsequent calls to the above setters will force-update the data
        m_last_texture = (texture_info *)~0;
        m_last_filter = -1;
        m_last_blendenable = -1;
        m_last_blendop = -1;
        m_last_blendsrc = -1;
        m_last_blenddst = -1;
        m_last_wrap = -1;
    }

	renderer::renderer(osd_window *window) :
		osd_renderer(window, FLAG_NONE),
        m_adapter_index(0),
		m_adapter(nullptr),
        m_output_index(0),
		m_output(nullptr),
        m_width(0),
        m_height(0),
        m_device(nullptr),
        m_device_context(nullptr),
        m_presentation({ 0 }),
        m_origmode({ 0 }),
        m_currentmode({ 0 }),
        m_pixformat(DXGI_FORMAT_B8G8R8A8_UNORM),
        m_vertexbuf(nullptr),
        m_lockedbuf(nullptr),
        m_numverts(0),
        m_vectorbatch(nullptr),
        m_batchindex(0),
        m_numpolys(0),
        m_restarting(false),
        m_last_texture(nullptr),
        m_last_texture_flags(0),
        m_last_blendenable(0),
        m_last_blendop(0),
        m_last_blendsrc(0),
        m_last_blenddst(0),
        m_last_filter(0),
        m_last_wrap(0),
        m_last_modmode(0),
        m_texture_manager(nullptr),
        m_line_count(0),
        m_shader_settings({ 0 }),
        m_vertex_shader(nullptr),
        m_pixel_shader(nullptr),
        m_shader_settings_buffer(nullptr),
        m_render_target_view(nullptr),
        m_sampler_state(nullptr),
		m_dxgiFactory(nullptr),
        m_d3dFeatureLevel(D3D_FEATURE_LEVEL_9_1),
        m_rendertarget_width(0),
        m_rendertarget_height(0),
        m_fullscreen(false),
        m_rotation(DXGI_MODE_ROTATION_UNSPECIFIED),
        m_layout(nullptr),
        m_swap_chain(nullptr),
		m_d3d_module(NULL),
		m_dxgi_module(NULL),
		m_pfnCreateDXGIFactory2(nullptr),
		m_pfnD3D11CreateDevice(nullptr)
	{
	}

	int renderer::initialize()
	{
		// configure the adapter for the mode we want
		if (config_adapter_mode())
			return false;

		// create the device immediately for the full screen case (defer for window mode)
		if (window().fullscreen() && device_create(window().m_focus_hwnd))
			return false;

		return true;
	}

    int renderer::pre_window_draw_check()
    {
        // if we're in the middle of resizing, leave things alone
        if (window().m_resize_state == RESIZE_STATE_RESIZING)
            return 0;

        // if we're restarting the renderer, leave things alone
        if (m_restarting)
        {
            m_restarting = false;
        }

        // if we have a device, check the cooperative level
        if (m_device != nullptr)
        {
            if (device_test_cooperative())
            {
                return 1;
            }
        }

        // in window mode, we need to track the window size
        if (!window().fullscreen() || m_device == nullptr)
        {
            // if the size changes, skip this update since the render target will be out of date
            if (update_window_size())
                return 0;

            // if we have no device, after updating the size, return an error so GDI can try
            if (m_device == NULL)
            {
                osd_printf_error("No device after updating windows size! Attempting GDI fallback.\n");
                return 1;
            }
        }

        return -1;
    }

	void renderer::begin_frame()
	{
		// Clear the view
		const float ClearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		m_device_context->ClearRenderTargetView(m_render_target_view.Get(), ClearColor);

		// Set the Render Targets
		// We have to do this on each frame due to the swapchain::present() actually unbinding the targets
		m_device_context->OMSetRenderTargets(1, m_render_target_view.GetAddressOf(), nullptr);

		window().m_primlist->acquire_lock();

		// first update any textures
		m_texture_manager->update_textures();

		// Vertex Shader
		m_device_context->VSSetShader(
			m_vertex_shader.Get(),
			nullptr,
			0);

		// Pixel Shader
		m_device_context->PSSetShader(
			m_pixel_shader.Get(),
			nullptr,
			0);

		m_lockedbuf = NULL;

		m_line_count = 0;

		// loop over primitives
		for (render_primitive *prim = window().m_primlist->first(); prim != NULL; prim = prim->next())
			if (prim->type == render_primitive::LINE && PRIMFLAG_GET_VECTOR(prim->flags))
				m_line_count++;
	}

    void renderer::process_primitives()
    {
        // Rotating index for vector time offsets
        for (render_primitive *prim = window().m_primlist->first(); prim != NULL; prim = prim->next())
        {
            switch (prim->type)
            {
            case render_primitive::LINE:
                if (PRIMFLAG_GET_VECTOR(prim->flags))
                {
                    if (m_line_count > 0)
                        batch_vectors();
                    else
                        continue;
                }
                else
                {
                    draw_line(prim);
                }
                break;

            case render_primitive::QUAD:
                draw_quad(prim);
                break;

            default:
                throw emu_fatalerror("Unexpected render_primitive type");
            }
        }
    }

	void renderer::end_frame()
	{
		window().m_primlist->release_lock();

		// flush any pending polygons
		primitive_flush_pending();

		// finish the scene
		// present the current buffers
		HRESULT result = m_swap_chain->Present(1, 0);
		if (result != S_OK) osd_printf_verbose("Direct3D: Error %08X during device present call\n", (unsigned int)result);
	}

    //============================================================
    //  device_create
    //============================================================

    int renderer::device_create(HWND device_hwnd)
    {
        // if a device exists, free it
        if (m_device != nullptr)
        {
            device_delete();
        }

        // This flag adds support for surfaces with a different color channel ordering than the API default.
        // You need it for compatibility with Direct2D.
        UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if defined(_DEBUG)
        if (SdkLayersAvailable())
        {
            // If the project is in a debug build, enable debugging via SDK Layers with this flag.
            creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
        }
#endif

        // This array defines the set of DirectX hardware feature levels this app supports.
        // The ordering is important and you should  preserve it.
        // Don't forget to declare your app's minimum required feature level in its
        // description.  All apps are assumed to support 9.1 unless otherwise stated.
        D3D_FEATURE_LEVEL featureLevels[] =
        {
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0,
            D3D_FEATURE_LEVEL_9_3,
            D3D_FEATURE_LEVEL_9_2,
            D3D_FEATURE_LEVEL_9_1
        };

        // Create The Direct3D Device and context
        ComPtr<ID3D11Device> device;
        ComPtr<ID3D11DeviceContext> context;

        HRESULT result = create_d3d11_device_dynamic(
            m_adapter.Get(),					// use the adapter we previously discovered
            D3D_DRIVER_TYPE_UNKNOWN,
            0,
            creationFlags,						// optionally set debug and Direct2D compatibility flags
            featureLevels,						// list of feature levels this app can support
            ARRAYSIZE(featureLevels),			// number of possible feature levels
            D3D11_SDK_VERSION,
            device.GetAddressOf(),				// returns the Direct3D device created
            &m_d3dFeatureLevel,					// returns feature level of device created
            context.GetAddressOf());			// returns the device immediate context

        if (FAILED(result))
        {
            osd_printf_error("Could not create D3D11 device: 0x%x\n", (unsigned int)result);
            return result;
        }

        ThrowIfFailed(device.As(&m_device));
        ThrowIfFailed(context.As(&m_device_context));

        // Name the Device and context
        NAME_OBJECT(m_device, "D3D 11 Device");
        NAME_OBJECT(m_device_context, "D3D 11 Context");

        // Verify the capabilities meet our requirements
        int verify = device_verify_caps();
        if (verify == 2)
        {
            osd_printf_error("Error: Device does not meet minimum requirements for Direct3D rendering\n");
            return 1;
        }

        if (verify == 1)
        {
            osd_printf_warning("Warning: Device may not perform well for Direct3D rendering\n");
        }

        // Only after we verify and process the capabilities can we initialize the texture manager
        // It needs info about the capabilities
        m_texture_manager = global_alloc(texture_manager(this));

        result = device_create_resources();
        if (result != 0)
        {
            return result;
        }

        return create_size_dependent_resources();
    }

    //============================================================
    //  device_create_resources
    //============================================================

    int renderer::device_create_resources()
    {
        // allocate a vertex buffer to use
        D3D11_BUFFER_DESC bufferDesc = { 0 };
        bufferDesc.ByteWidth = sizeof(vertex) * VERTEX_BUFFER_SIZE;
        bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

        // Allocate a vertex buffer
        ThrowIfFailed(
            m_device->CreateBuffer(
                &bufferDesc,
                nullptr,
                m_vertexbuf.GetAddressOf()));

        NAME_OBJECT(m_vertexbuf, "Vertex Buffer");

        // set the stream
        UINT stride = sizeof(vertex);
        UINT offset = 0;
        m_device_context->IASetVertexBuffers(0, 1, m_vertexbuf.GetAddressOf(), &stride, &offset);

        // Create the vertex shader
        ThrowIfFailed(
            m_device->CreateVertexShader(
                d3d11_SimpleVertexShader,  //vertexShaderData.get(),
                sizeof(d3d11_SimpleVertexShader), //vertexShaderLength,
                nullptr,
                m_vertex_shader.GetAddressOf()));

        NAME_OBJECT(m_vertex_shader, "Vertex Shader");

        // Describe the layout
        D3D11_INPUT_ELEMENT_DESC layout[] =
        {
            { "POSITION",	0,	DXGI_FORMAT_R32G32B32_FLOAT,	0,	0,	D3D11_INPUT_PER_VERTEX_DATA,	0 },
            { "RHW",		0,	DXGI_FORMAT_R32_FLOAT,			0,	12,	D3D11_INPUT_PER_VERTEX_DATA,	0 },
            { "COLOR",		0,	DXGI_FORMAT_R32G32B32A32_FLOAT,	0,	16,	D3D11_INPUT_PER_VERTEX_DATA,	0 },
            { "TEXCOORD",	0,	DXGI_FORMAT_R32G32_FLOAT,		0,	32, D3D11_INPUT_PER_VERTEX_DATA,	0 },
        };

        ThrowIfFailed(
            m_device->CreateInputLayout(
                layout,
                ARRAYSIZE(layout),
                d3d11_SimpleVertexShader,  //vertexShaderData.get(),
                sizeof(d3d11_SimpleVertexShader), //vertexShaderLength,
                &m_layout));

        NAME_OBJECT(m_layout, "Input Layout");

        // Set the Input layout
        m_device_context->IASetInputLayout(m_layout.Get());

        // Create the pixel shader
        ThrowIfFailed(
            m_device->CreatePixelShader(
                d3d11_PixelShader, // pixelShaderData.get(),
                sizeof(d3d11_PixelShader), // pixelShaderLength,
                nullptr,
                &m_pixel_shader));

        NAME_OBJECT(m_pixel_shader, "Pixel Shader");

        return 0;
    }

    int renderer::create_size_dependent_resources()
    {
        // The width and height of the swap chain must be based on the window's
        // landscape-oriented width and height. If the window is in a portrait
        // orientation, the dimensions must be reversed.

        bool swapDimensions =
            m_rotation == DXGI_MODE_ROTATION_ROTATE90 ||
            m_rotation == DXGI_MODE_ROTATION_ROTATE270;

        m_rendertarget_width = swapDimensions ? m_height : m_width;
        m_rendertarget_height = swapDimensions ? m_width : m_height;

        if (m_swap_chain != nullptr)
        {
            // If the swap chain already exists, resize it if needed
            ThrowIfFailed(m_swap_chain->GetDesc1(&m_presentation));
            if (m_presentation.Height != m_rendertarget_height || m_presentation.Width != m_rendertarget_width)
            {
                ThrowIfFailed(
                    m_swap_chain->ResizeBuffers(
                        2, // Double-buffered swap chain.
                        static_cast<UINT>(m_rendertarget_width),
                        static_cast<UINT>(m_rendertarget_height),
                        (m_pixformat == DXGI_FORMAT_UNKNOWN) ? DXGI_FORMAT_B8G8R8A8_UNORM : m_pixformat,
                        window().fullscreen() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH : 0));
            }
        }
        else
        {
            // initialize the D3D presentation parameters
            memset(&m_presentation, 0, sizeof(m_presentation));
            m_presentation.Width = m_rendertarget_width;
            m_presentation.Height = m_rendertarget_height;
            m_presentation.Format = (m_pixformat == DXGI_FORMAT_UNKNOWN) ? DXGI_FORMAT_B8G8R8A8_UNORM : m_pixformat;
            m_presentation.BufferCount = 2; // with D3D11, we always have to use a double buffered swapchain
            m_presentation.SampleDesc.Count = 1; // Don't use multi-sampling (anti-alias)
            m_presentation.SampleDesc.Quality = 0;
            m_presentation.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
            m_presentation.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            m_presentation.Flags = window().fullscreen() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH : 0;

            DXGI_SWAP_CHAIN_FULLSCREEN_DESC *pFullScreenDesc = nullptr;

            // If we want to be in full screen mode, provide a FULLSCREEN_DESC with the ability to switch
            if (window().fullscreen())
            {
                DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreen_desc = { 0 };
                fullscreen_desc.RefreshRate = m_currentmode.RefreshRate;
                fullscreen_desc.Scaling = m_currentmode.Scaling;
                fullscreen_desc.ScanlineOrdering = m_currentmode.ScanlineOrdering;
                fullscreen_desc.Windowed = TRUE;

                pFullScreenDesc = &fullscreen_desc;
            }

            // Create the swap chain
            ThrowIfFailed(
                m_dxgiFactory->CreateSwapChainForHwnd(
                    m_device.Get(),
                    window().m_hwnd,
                    &m_presentation,
                    pFullScreenDesc,
                    nullptr,
                    m_swap_chain.GetAddressOf()));

            NAME_OBJECT(m_swap_chain, "SwapChain");

            // We want to be in full screen but aren't
            if (!m_fullscreen && window().fullscreen())
            {
                ThrowIfFailed(m_swap_chain->SetFullscreenState(TRUE, nullptr));
                ThrowIfFailed(m_swap_chain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

                // Remember new full screen mode
                ComPtr<IDXGIOutput> fullscreen_output;
                ThrowIfFailed(m_swap_chain->GetFullscreenState(&m_fullscreen, fullscreen_output.GetAddressOf()));

                // Save off the full screen adapter
                if (fullscreen_output != nullptr)
                {
                    m_output = fullscreen_output;
                }
            }
        }

        // Gamma
        if (window().fullscreen())
        {
            set_fullscreen_gamma();
        }

        // Get the DXGI Device
        ComPtr<IDXGIDevice2> dxgiDevice;
        ThrowIfFailed(m_device.As(&dxgiDevice));

        // Ensure that DXGI does not queue more than one frame at a time. This both reduces latency and
        // ensures that the application will only render after each VSync, minimizing power consumption.
        ThrowIfFailed(dxgiDevice->SetMaximumFrameLatency(1));

        m_shader_settings.PostPass = 0.0f;
        m_shader_settings.TargetHeight = m_rendertarget_height;
        m_shader_settings.TargetWidth = m_rendertarget_width;

        D3D11_SUBRESOURCE_DATA shaderSettingsData;
        shaderSettingsData.pSysMem = &m_shader_settings;
        shaderSettingsData.SysMemPitch = 0;
        shaderSettingsData.SysMemSlicePitch = 0;

        CD3D11_BUFFER_DESC constantBufferDesc(sizeof(m_shader_settings), D3D11_BIND_CONSTANT_BUFFER);

        ThrowIfFailed(
            m_device->CreateBuffer(
                &constantBufferDesc,
                &shaderSettingsData,
                m_shader_settings_buffer.GetAddressOf()));

        NAME_OBJECT(m_shader_settings_buffer, "Shader Settings Buffer");

        // Set Vertex Shader constant buffer
        m_device_context->VSSetConstantBuffers(0, 1, m_shader_settings_buffer.GetAddressOf());

        // Create a render target view of the swap chain back buffer
        ComPtr<ID3D11Texture2D> backBuffer;
        ThrowIfFailed(
            m_swap_chain->GetBuffer(
                0,
                __uuidof(ID3D11Texture2D),
                reinterpret_cast<void**>(backBuffer.GetAddressOf())
                )
            );

        NAME_OBJECT(backBuffer, "Back Buffer");

        // Create the render target view
        ThrowIfFailed(
            m_device->CreateRenderTargetView(
                (ID3D11Texture2D *)backBuffer.Get(),
                nullptr,
                m_render_target_view.ReleaseAndGetAddressOf()));

        NAME_OBJECT(m_render_target_view, "Render Target View");

        backBuffer.Reset();

        // Set the rendering viewport to target the entire window.
        D3D11_VIEWPORT viewport = { 0 };
        viewport.TopLeftX = 0.0f;
        viewport.TopLeftY = 0.0f;
        viewport.Width = m_rendertarget_width;
        viewport.Height = m_rendertarget_height;
        viewport.MinDepth = D3D11_DEFAULT_VIEWPORT_MIN_DEPTH;
        viewport.MaxDepth = D3D11_DEFAULT_VIEWPORT_MAX_DEPTH;

        m_device_context->RSSetViewports(1, &viewport);

        // reset the local states to force updates
        reset_render_states();

        // clear the buffer
        const FLOAT clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        m_device_context->ClearRenderTargetView(m_render_target_view.Get(), clearColor);
        m_swap_chain->Present(1, 0);

        m_texture_manager->create_resources();

        return 0;
    }

    //============================================================
    //  device_delete
    //============================================================

    renderer::~renderer()
    {
        device_delete();
    }

    void renderer::device_delete()
    {
        if (m_swap_chain != nullptr)
        {
            m_swap_chain->SetFullscreenState(FALSE, nullptr);
            m_swap_chain.Reset();
        }

        // free our base resources
        device_delete_resources();

        if (m_texture_manager != NULL)
        {
            global_free(m_texture_manager);
            m_texture_manager = NULL;
        }

        // free the device itself
        m_device.Reset();
        m_device_context.Reset();
    }

    //============================================================
    //  device_delete_resources
    //============================================================

    void renderer::device_delete_resources()
    {
        if (m_texture_manager != NULL)
        {
            m_texture_manager->delete_resources();
        }

        m_vertex_shader.Reset();
        m_layout.Reset();
        m_pixel_shader.Reset();
        m_vertexbuf.Reset();
    }

    //============================================================
    //  device_verify_caps
    //============================================================

    int renderer::device_verify_caps()
    {
        // Device doesn't meet minimum feature level
        if (m_d3dFeatureLevel < D3D_FEATURE_LEVEL_9_1)
            return 2;

        // Device is a feature-level 9.X and won't perform as well
        if (m_d3dFeatureLevel < D3D_FEATURE_LEVEL_10_0)
            return 1;

        return 0;
    }

    //============================================================
    //  device_test_cooperative
    //============================================================

    int renderer::device_test_cooperative()
    {
        // if we have a device, check the cooperative level
        if (this->m_device != nullptr && this->m_swap_chain != nullptr)
        {
            int error = m_swap_chain->Present(1, DXGI_PRESENT_TEST);
            if (error)
                return 1;
        }

        return 0;
    }

    //============================================================
    //  config_adapter_mode
    //============================================================

    int renderer::config_adapter_mode()
    {
        // There is no reason to do this again
        if (m_dxgiFactory != nullptr)
        {
            return 0;
        }

        UINT factory_flags = 0;

#ifdef _DEBUG
        factory_flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

        // initialize the factory
        ThrowIfFailed(create_dxgi_factory2_dynamic(
            factory_flags,
            __uuidof(IDXGIFactory2),
            m_dxgiFactory.GetAddressOf()));

        // choose the monitor number
        HRESULT result = get_adapter_and_output_for_monitor();
        if (result != S_OK)
        {
            osd_printf_error("Error getting adapter information: 0x%x\n", (unsigned int)result);
            return 1;
        }

        DXGI_OUTPUT_DESC output_desc;
        result = m_output->GetDesc(&output_desc);
        if (FAILED(result))
        {
            osd_printf_error("Error getting adapter descriptor: 0x%x\n", (unsigned int)result);
            return 1;
        }

        osd_printf_verbose("Direct3D: Configuring adapter #%d = %S\n", m_adapter_index, output_desc.DeviceName);

        // Store the rotation state
        m_rotation = output_desc.Rotation;

        unique_ptr<DXGI_MODE_DESC[]> display_modes;
        UINT mode_count = 0;
        result = get_output_modes(mode_count, display_modes);
        if (FAILED(result))
        {
            osd_printf_error("Error getting display mode list for output: 0x%x\n", (unsigned int)result);
            return 1;
        }

        osd_printf_verbose("Number of modes supported: %u\n", mode_count);

        // Compute screen height and width from the desktop coordinates
        LONG screenWidth = output_desc.DesktopCoordinates.right - output_desc.DesktopCoordinates.left;
        LONG screenHeight = output_desc.DesktopCoordinates.bottom - output_desc.DesktopCoordinates.top;

        // Find a matching display mode
        for (int i = 0; i < mode_count; i++)
        {
            if (display_modes[i].Width == (unsigned int)screenWidth)
            {
                if (display_modes[i].Height == (unsigned int)screenHeight)
                {
                    m_origmode = display_modes[i];
                    break;
                }
            }
        }

        // We'll stay in the original mode
        m_currentmode = m_origmode;

        // The texture format from the mode
        m_pixformat = m_origmode.Format;

        // choose a resolution: window mode case
        if (!window().fullscreen() || !video_config.switchres || window().win_has_menu())
        {
            RECT client;

            // bounds are from the window client rect
            GetClientRectExceptMenu(window().m_hwnd, &client, window().fullscreen());
            m_width = client.right - client.left;
            m_height = client.bottom - client.top;

            // make sure it's a pixel format we can get behind
            if (m_pixformat != DXGI_FORMAT_B5G6R5_UNORM
                && m_pixformat != DXGI_FORMAT_B8G8R8X8_UNORM
                && m_pixformat != DXGI_FORMAT_B8G8R8A8_UNORM)
            {
                osd_printf_error("Device %s currently in an unsupported mode. Format: %u\n", window().monitor()->devicename(), (unsigned int)m_pixformat);
                return 1;
            }
        }
        else
        {
            // default to the current mode dimensions exactly
            m_width = m_currentmode.Width;
            m_height = m_currentmode.Height;

            // if we're allowed to switch resolutions, override with something better
            if (video_config.switchres)
                pick_best_mode(mode_count, display_modes);
        }

        return 0;
    }

    //============================================================
    //  get_adapter_for_monitor
    //============================================================

    int renderer::get_adapter_and_output_for_monitor()
    {
        HRESULT result = S_OK;
        ComPtr<IDXGIOutput> first_output;
        ComPtr<IDXGIAdapter1> first_adapter;

        ComPtr<IDXGIAdapter1> currentAdapter;
        for (UINT adapternum = 0; ; adapternum++)
        {
            result = m_dxgiFactory->EnumAdapters1(adapternum, currentAdapter.ReleaseAndGetAddressOf());

            // This error simply means we hit the end
            if (result == DXGI_ERROR_NOT_FOUND)
                break;

            // Fail on any other errors
            if (FAILED(result))
            {
                return result;
            }

            DXGI_ADAPTER_DESC1 adapterDesc;
            result = currentAdapter->GetDesc1(&adapterDesc);
            if (FAILED(result)
                || (adapterDesc.Flags & DXGI_ADAPTER_FLAG_REMOTE)		// We don't want remote devices
                || (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE))	// We don't want software devices
            {
                osd_printf_verbose("Skipping non-hardware adapter #%u - %S\n", adapternum, adapterDesc.Description);
                continue;
            }

            // Save off the first adapter for a fallback
            if (adapternum == 0)
            {
                first_adapter = currentAdapter;
            }

            ComPtr<IDXGIOutput> currentOutput;
            for (UINT outputnum = 0; ; outputnum++)
            {
                result = currentAdapter->EnumOutputs(outputnum, currentOutput.ReleaseAndGetAddressOf());

                // This error simply means we hit the end
                if (result == DXGI_ERROR_NOT_FOUND)
                    break;

                // Fail on any other errors
                if (FAILED(result))
                {
                    return result;
                }

                // Special case, save off the first output
                // This is in case we can't find the target one and default to 0
                if (adapternum == 0 && outputnum == 0)
                {
                    first_output = currentOutput;
                }

                DXGI_OUTPUT_DESC output_desc;
                result = currentOutput->GetDesc(&output_desc);
                if (FAILED(result))
                {
                    return result;
                }

                if (output_desc.Monitor == *((HMONITOR *)window().monitor()->oshandle()))
                {
                    // We found the monitor, set the output
                    m_adapter_index = adapternum;
                    m_adapter = currentAdapter;
                    m_output_index = outputnum;
                    m_output = currentOutput;

                    return S_OK;
                }
            }
        }

        // If we didn't find the adapter, get the first one
        m_adapter_index = 0;
        m_adapter = first_adapter;
        m_output_index = 0;
        m_output = first_output;

        osd_printf_verbose("Could not find the matching adapter and output. Falling back to adapter/output 0/0\n");

        return S_OK;
    }

    //============================================================
    //  pick_best_mode
    //============================================================

    void renderer::pick_best_mode(_In_ UINT mode_count, _In_ const std::unique_ptr<DXGI_MODE_DESC[]> &modes)
    {
        double target_refresh = 60.0;
        INT32 minwidth, minheight;
        float best_score = 0.0f;

        // determine the refresh rate of the primary screen
        const screen_device *primary_screen = window().machine().config().first_screen();
        if (primary_screen != NULL)
        {
            target_refresh = ATTOSECONDS_TO_HZ(primary_screen->refresh_attoseconds());
        }

        // determine the minimum width/height for the selected target
        // note: technically we should not be calling this from an alternate window
        // thread; however, it is only done during init time, and the init code on
        // the main thread is waiting for us to finish, so it is safe to do so here
        window().target()->compute_minimum_size(minwidth, minheight);

        // use those as the target for now
        INT32 target_width = minwidth;
        INT32 target_height = minheight;

        // enumerate all the video modes and find the best match
        osd_printf_verbose("Direct3D: Selecting video mode...\n");
        for (int modenum = 0; modenum < mode_count; modenum++)
        {
            // check this mode
            DXGI_MODE_DESC mode = modes[modenum];

            // skip non-32 bit modes
            if (mode.Format != DXGI_FORMAT_B8G8R8X8_UNORM && mode.Format != DXGI_FORMAT_B8G8R8A8_UNORM)
                continue;

            // compute initial score based on difference between target and current
            float size_score = 1.0f / (1.0f + fabs((float)(mode.Width - target_width)) + fabs((float)(mode.Height - target_height)));

            // if the mode is too small, give a big penalty
            if (mode.Width < minwidth || mode.Height < minheight)
                size_score *= 0.01f;

            // if mode is smaller than we'd like, it only scores up to 0.1
            if (mode.Width < target_width || mode.Height < target_height)
                size_score *= 0.1f;

            // if we're looking for a particular mode, that's a winner
            if (mode.Width == window().m_win_config.width && mode.Height == window().m_win_config.height)
                size_score = 2.0f;

            // compute refresh score
            double mode_refresh_rate = (double)mode.RefreshRate.Numerator / mode.RefreshRate.Denominator;
            float refresh_score = 1.0f / (1.0f + fabs(mode_refresh_rate - target_refresh));

            // if refresh is smaller than we'd like, it only scores up to 0.1
            if (mode_refresh_rate < target_refresh)
                refresh_score *= 0.1f;

            // if we're looking for a particular refresh, make sure it matches
            if (mode_refresh_rate == window().m_win_config.refresh)
                refresh_score = 2.0f;

            // weight size and refresh equally
            float final_score = size_score + refresh_score;

            // best so far?
            osd_printf_verbose("  %4dx%4d@%3dHz -> %f\n", mode.Width, mode.Height, (int)round(mode_refresh_rate), final_score * 1000.0f);
            if (final_score > best_score)
            {
                best_score = final_score;
                m_width = mode.Width;
                m_height = mode.Height;
                m_pixformat = mode.Format; // REVIEW: Do we want the pix format from the mode?

                m_currentmode = mode;
            }
        }

        osd_printf_verbose("Direct3D: Mode selected = %4dx%4d@%3dHz\n", m_width, m_height, (int)round(refresh_as_float(m_origmode.RefreshRate)));
    }

    int renderer::get_output_modes(_Out_ UINT &mode_count, _Out_ unique_ptr<DXGI_MODE_DESC[]> &modes)
    {
        // Call this the first time to get the number of modes
        UINT numModes;
        m_output->GetDisplayModeList(
            DXGI_FORMAT_B8G8R8A8_UNORM,
            DXGI_ENUM_MODES_INTERLACED,
            &numModes,
            NULL);

        // now allocate our array and 
        auto temp_modes = make_unique<DXGI_MODE_DESC[]>(numModes);
        HRESULT result = m_output->GetDisplayModeList(
            DXGI_FORMAT_B8G8R8A8_UNORM,
            DXGI_ENUM_MODES_INTERLACED,
            &numModes,
            temp_modes.get());

        if (FAILED(result))
        {
            osd_printf_error("GetDisplayModeList failed with status 0x%x\n", (unsigned int)result);
            return result;
        }

        // Assign the output and return
        mode_count = numModes;
        modes = std::move(temp_modes);

        osd_printf_verbose("renderer::get_output_modes - number of modes: %u\n", numModes);
        return result;
    }

    //============================================================
    //  update_window_size
    //============================================================

    BOOL renderer::update_window_size()
    {
        // get the current window bounds
        RECT client;
        GetClientRectExceptMenu(window().m_hwnd, &client, window().fullscreen());

        // if we have a device and matching width/height, nothing to do
        if (m_device != NULL && rect_width(&client) == m_width && rect_height(&client) == m_height)
        {
            // clear out any pending resizing if the area didn't change
            if (window().m_resize_state == RESIZE_STATE_PENDING)
                window().m_resize_state = RESIZE_STATE_NORMAL;
            return FALSE;
        }

        // if we're in the middle of resizing, leave it alone as well
        if (window().m_resize_state == RESIZE_STATE_RESIZING)
            return FALSE;

        // set the new bounds and create the device again
        m_width = rect_width(&client);
        m_height = rect_height(&client);
        if (device_create(window().m_focus_hwnd))
            return FALSE;

        // reset the resize state to normal, and indicate we made a change
        window().m_resize_state = RESIZE_STATE_NORMAL;
        return TRUE;
    }

	//============================================================
	//  batch_vectors
	//============================================================

	void renderer::batch_vectors()
	{
		windows_options &options = downcast<windows_options &>(window().machine().options());

		int vector_size = (options.antialias() ? 24 : 6);
		m_vectorbatch = mesh_alloc(m_line_count * vector_size);
		m_batchindex = 0;

		static int start_index = 0;
		int line_index = 0;
		float period = options.screen_vector_time_period();
		UINT32 cached_flags = 0;
		for (render_primitive *prim = window().m_primlist->first(); prim != NULL; prim = prim->next())
		{
			switch (prim->type)
			{
			case render_primitive::LINE:
				if (PRIMFLAG_GET_VECTOR(prim->flags))
				{
					if (period == 0.0f || m_line_count == 0)
					{
						batch_vector(prim, 1.0f);
					}
					else
					{
						batch_vector(prim, (float)(start_index + line_index) / ((float)m_line_count * period));
						line_index++;
					}
					cached_flags = prim->flags;
				}
				break;

			default:
				// Skip
				break;
			}
		}

		// now add a polygon entry
		m_poly[m_numpolys].init(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, m_line_count * (options.antialias() ? 8 : 2), vector_size * m_line_count, cached_flags,
			m_texture_manager->get_vector_texture(), D3DTOP_MODULATE, 0.0f, 1.0f, 0.0f, 0.0f);
		m_numpolys++;

		start_index += (int)((float)line_index * period);
		if (m_line_count > 0)
		{
			start_index %= m_line_count;
		}

		m_line_count = 0;
	}

	void renderer::batch_vector(const render_primitive *prim, float line_time)
	{
		// compute the effective width based on the direction of the line
		float effwidth = prim->width;
		if (effwidth < 0.5f)
		{
			effwidth = 0.5f;
		}

		// determine the bounds of a quad to draw this line
		render_bounds b0, b1;
		render_line_to_quad(&prim->bounds, effwidth, &b0, &b1);

		// iterate over AA steps
		for (const line_aa_step *step = PRIMFLAG_GET_ANTIALIAS(prim->flags) ? line_aa_4step : line_aa_1step;
		step->weight != 0; step++)
		{
			// get a pointer to the vertex buffer
			if (m_vectorbatch == NULL)
				return;

			m_vectorbatch[m_batchindex + 0].x = b0.x0 + step->xoffs;
			m_vectorbatch[m_batchindex + 0].y = b0.y0 + step->yoffs;
			m_vectorbatch[m_batchindex + 1].x = b0.x1 + step->xoffs;
			m_vectorbatch[m_batchindex + 1].y = b0.y1 + step->yoffs;
			m_vectorbatch[m_batchindex + 2].x = b1.x0 + step->xoffs;
			m_vectorbatch[m_batchindex + 2].y = b1.y0 + step->yoffs;

			m_vectorbatch[m_batchindex + 3].x = b0.x1 + step->xoffs;
			m_vectorbatch[m_batchindex + 3].y = b0.y1 + step->yoffs;
			m_vectorbatch[m_batchindex + 4].x = b1.x0 + step->xoffs;
			m_vectorbatch[m_batchindex + 4].y = b1.y0 + step->yoffs;
			m_vectorbatch[m_batchindex + 5].x = b1.x1 + step->xoffs;
			m_vectorbatch[m_batchindex + 5].y = b1.y1 + step->yoffs;

			float dx = b1.x1 - b0.x1;
			float dy = b1.y1 - b0.y1;
			float line_length = sqrtf(dx * dx + dy * dy);

			// determine the color of the line
			float r = prim->color.r * step->weight;
			float g = prim->color.g * step->weight;
			float b = prim->color.b * step->weight;
			float a = prim->color.a;

			// REVIEW: this logic was ported from bit-shift from the original
			if (r > 1.0 || g > 1.0 || b > 1.0)
			{
				float shift = r > 2.0 || g > 2.0 || b > 2.0 ? 4 : 1;
				r /= shift; g /= shift; b /= shift;
			}
			if (r > 1.0) r = 1.0;
			if (g > 1.0) g = 1.0;
			if (b > 1.0) b = 1.0;
			if (a > 1.0) a = 1.0;
			D3DXVECTOR4 color = D3DXVECTOR4(r, g, b, a);

			vec2f& start = m_texture_manager->get_vector_texture()
				? m_texture_manager->get_vector_texture()->get_uvstart()
				: m_texture_manager->get_default_texture()->get_uvstart();

			vec2f& stop = m_texture_manager->get_vector_texture()
				? m_texture_manager->get_vector_texture()->get_uvstop()
				: m_texture_manager->get_default_texture()->get_uvstop();

			m_vectorbatch[m_batchindex + 0].u0 = start.c.x;
			m_vectorbatch[m_batchindex + 0].v0 = start.c.y;
			m_vectorbatch[m_batchindex + 1].u0 = start.c.x;
			m_vectorbatch[m_batchindex + 1].v0 = stop.c.y;
			m_vectorbatch[m_batchindex + 2].u0 = stop.c.x;
			m_vectorbatch[m_batchindex + 2].v0 = start.c.y;

			m_vectorbatch[m_batchindex + 3].u0 = start.c.x;
			m_vectorbatch[m_batchindex + 3].v0 = stop.c.y;
			m_vectorbatch[m_batchindex + 4].u0 = stop.c.x;
			m_vectorbatch[m_batchindex + 4].v0 = start.c.y;
			m_vectorbatch[m_batchindex + 5].u0 = stop.c.x;
			m_vectorbatch[m_batchindex + 5].v0 = stop.c.y;

			m_vectorbatch[m_batchindex + 0].u1 = line_length;
			m_vectorbatch[m_batchindex + 1].u1 = line_length;
			m_vectorbatch[m_batchindex + 2].u1 = line_length;
			m_vectorbatch[m_batchindex + 3].u1 = line_length;
			m_vectorbatch[m_batchindex + 4].u1 = line_length;
			m_vectorbatch[m_batchindex + 5].u1 = line_length;

			// set the color, Z parameters to standard values
			for (int i = 0; i < 6; i++)
			{
				m_vectorbatch[m_batchindex + i].z = 0.0f;
				m_vectorbatch[m_batchindex + i].rhw = 1.0f;
				m_vectorbatch[m_batchindex + i].color = color;
			}

			m_batchindex += 6;
		}
	}

	//============================================================
	//  draw_line
	//============================================================

	void renderer::draw_line(const render_primitive *prim)
	{
		// compute the effective width based on the direction of the line
		float effwidth = prim->width;
		if (effwidth < 0.5f)
		{
			effwidth = 0.5f;
		}

		// determine the bounds of a quad to draw this line
		render_bounds b0, b1;
		render_line_to_quad(&prim->bounds, effwidth, &b0, &b1);

		// iterate over AA steps
		for (const line_aa_step *step = PRIMFLAG_GET_ANTIALIAS(prim->flags) ? line_aa_4step : line_aa_1step;
		step->weight != 0; step++)
		{
			// get a pointer to the vertex buffer
			vertex *vertex = mesh_alloc(4);
			if (vertex == NULL)
				return;

			// rotate the unit vector by 135 degrees and add to point 0
			vertex[0].x = b0.x0 + step->xoffs;
			vertex[0].y = b0.y0 + step->yoffs;

			// rotate the unit vector by -135 degrees and add to point 0
			vertex[1].x = b0.x1 + step->xoffs;
			vertex[1].y = b0.y1 + step->yoffs;

			// rotate the unit vector by 45 degrees and add to point 1
			vertex[2].x = b1.x0 + step->xoffs;
			vertex[2].y = b1.y0 + step->yoffs;

			// rotate the unit vector by -45 degrees and add to point 1
			vertex[3].x = b1.x1 + step->xoffs;
			vertex[3].y = b1.y1 + step->yoffs;

			// determine the color of the line
			float r = prim->color.r * step->weight;
			float g = prim->color.g * step->weight;
			float b = prim->color.b * step->weight;
			float a = prim->color.a;
			if (r > 1.0) r = 1.0;
			if (g > 1.0) g = 1.0;
			if (b > 1.0) b = 1.0;
			if (a > 1.0) a = 1.0;
			D3DXVECTOR4 color = D3DXVECTOR4(r, g, b, a);

			vec2f& start = m_texture_manager->get_vector_texture()
				? m_texture_manager->get_vector_texture()->get_uvstart()
				: m_texture_manager->get_default_texture()->get_uvstart();

			vec2f& stop = m_texture_manager->get_vector_texture()
				? m_texture_manager->get_vector_texture()->get_uvstop()
				: m_texture_manager->get_default_texture()->get_uvstop();

			vertex[0].u0 = start.c.x;
			vertex[0].v0 = start.c.y;

			vertex[2].u0 = stop.c.x;
			vertex[2].v0 = start.c.y;

			vertex[1].u0 = start.c.x;
			vertex[1].v0 = stop.c.y;

			vertex[3].u0 = stop.c.x;
			vertex[3].v0 = stop.c.y;

			// set the color, Z parameters to standard values
			for (int i = 0; i < 4; i++)
			{
				vertex[i].z = 0.0f;
				vertex[i].rhw = 1.0f;
				vertex[i].color = color;
			}

			// now add a polygon entry
			m_poly[m_numpolys].init(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, 2, 4, prim->flags, m_texture_manager->get_vector_texture(),
				D3DTOP_MODULATE, 0.0f, 1.0f, 0.0f, 0.0f);
			m_numpolys++;
		}
	}

	//============================================================
	//  draw_quad
	//============================================================

	void renderer::draw_quad(const render_primitive *prim)
	{
		texture_info *texture = m_texture_manager->find_texinfo(&prim->texture, prim->flags);

		if (texture == NULL)
		{
			texture = m_texture_manager->get_default_texture();
		}

		// get a pointer to the vertex buffer
		vertex *vertex = mesh_alloc(4);
		if (vertex == NULL)
			return;

		// fill in the vertexes clockwise
		vertex[0].x = prim->bounds.x0;
		vertex[0].y = prim->bounds.y0;
		vertex[1].x = prim->bounds.x1;
		vertex[1].y = prim->bounds.y0;
		vertex[2].x = prim->bounds.x0;
		vertex[2].y = prim->bounds.y1;
		vertex[3].x = prim->bounds.x1;
		vertex[3].y = prim->bounds.y1;
		float width = prim->bounds.x1 - prim->bounds.x0;
		float height = prim->bounds.y1 - prim->bounds.y0;

		// set the texture coordinates
		if (texture != NULL)
		{
			vec2f& start = texture->get_uvstart();
			vec2f& stop = texture->get_uvstop();
			vec2f delta = stop - start;
			vertex[0].u0 = start.c.x + delta.c.x * prim->texcoords.tl.u;
			vertex[0].v0 = start.c.y + delta.c.y * prim->texcoords.tl.v;
			vertex[1].u0 = start.c.x + delta.c.x * prim->texcoords.tr.u;
			vertex[1].v0 = start.c.y + delta.c.y * prim->texcoords.tr.v;
			vertex[2].u0 = start.c.x + delta.c.x * prim->texcoords.bl.u;
			vertex[2].v0 = start.c.y + delta.c.y * prim->texcoords.bl.v;
			vertex[3].u0 = start.c.x + delta.c.x * prim->texcoords.br.u;
			vertex[3].v0 = start.c.y + delta.c.y * prim->texcoords.br.v;
		}

		// determine the color, allowing for over modulation
		float r = prim->color.r;
		float g = prim->color.g;
		float b = prim->color.b;
		float a = prim->color.a;

		DWORD modmode = D3DTOP_MODULATE;
		// Some info I found on this:
		// https://code.msdn.microsoft.com/windowsdesktop/Effects-11-Win32-Samples-cce82a4d
		// Multi-Texturing
		// The texture stages from the fixed - function pipeline are officially gone.
		// In there place is the ability to load textures arbitrarily in the pixel shader and to 
		// combine them in any way that the math operations of the language allow
		// For D3DTOP_MODULATE, the shader would simply multiply the colors together instead of adding them

		//if (texture != NULL)
		//{
		//	if (m_mod2x_supported && (r > 255 || g > 255 || b > 255))
		//	{
		//		if (m_mod4x_supported && (r > 2 * 255 || g > 2 * 255 || b > 2 * 255))
		//		{
		//			r >>= 2; g >>= 2; b >>= 2;
		//			modmode = D3DTOP_MODULATE4X;
		//		}
		//		else
		//		{
		//			r >>= 1; g >>= 1; b >>= 1;
		//			modmode = D3DTOP_MODULATE2X;
		//		}
		//	}
		//}*/

		if (r > 1.0) r = 1.0;
		if (g > 1.0) g = 1.0;
		if (b > 1.0) b = 1.0;
		if (a > 1.0) a = 1.0;
		D3DXVECTOR4 color = D3DXVECTOR4(r, g, b, a);

		// adjust half pixel X/Y offset, set the color, Z parameters to standard values
		for (int i = 0; i < 4; i++)
		{
			// REVIEW: I don't think we need the adjustment here
			//vertex[i].x -= 0.5f;
			//vertex[i].y -= 0.5f;
			vertex[i].z = 0.0f;
			vertex[i].rhw = 1.0f;
			vertex[i].color = color;
		}

		// now add a polygon entry
		m_poly[m_numpolys].init(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, 2, 4, prim->flags, texture, modmode, width, height);
		m_numpolys++;
	}

	//============================================================
	//  primitive_alloc
	//============================================================

	vertex *renderer::mesh_alloc(int numverts)
	{
		HRESULT result;

		// if we're going to overflow, flush
		if (m_lockedbuf != NULL && m_numverts + numverts >= VERTEX_BUFFER_SIZE)
		{
			primitive_flush_pending();
		}

		// if we don't have a lock, grab it now
		if (m_lockedbuf == NULL)
		{
			D3D11_MAPPED_SUBRESOURCE resource;
			result = m_device_context->Map(m_vertexbuf.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &resource);
			if (result != S_OK)
			{
				return NULL;
			}

			m_lockedbuf = static_cast<vertex*>(resource.pData);
		}

		// if we already have the lock and enough room, just return a pointer
		if (m_lockedbuf != NULL && m_numverts + numverts < VERTEX_BUFFER_SIZE)
		{
			int oldverts = m_numverts;
			m_numverts += numverts;
			return &m_lockedbuf[oldverts];
		}
		return NULL;
	}

	//============================================================
	//  primitive_flush_pending
	//============================================================

	void renderer::primitive_flush_pending()
	{
		// ignore if we're not locked
		if (m_lockedbuf == NULL)
		{
			return;
		}

		// unmap the buffer
		m_device_context->Unmap(m_vertexbuf.Get(), 0);
		m_lockedbuf = NULL;

		// m_shaders->begin_draw();

		int vertnum = 0;
		/*if (m_shaders->enabled())
		{
			vertnum = 6;
		}*/

		// now do the polys
		for (int polynum = 0; polynum < m_numpolys; polynum++)
		{
			UINT32 flags = m_poly[polynum].get_flags();
			texture_info *texture = m_poly[polynum].get_texture();
			int newfilter;

			// set the texture if different
			set_texture(texture);

			// set filtering if different
			if (texture != NULL)
			{
				newfilter = FALSE;
				if (PRIMFLAG_GET_SCREENTEX(flags))
					newfilter = video_config.filter;

				// set_sampler does filter and wrap
				set_sampler(
					newfilter,
					PRIMFLAG_GET_TEXWRAP(flags) ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP);

				//set_modmode(m_poly[polynum].get_modmode());

				//m_shaders->init_effect_info(&m_poly[polynum]);
			}

			// set the blendmode if different
			set_blendmode(PRIMFLAG_GET_BLENDMODE(flags));

			if (vertnum + m_poly[polynum].get_vertcount() > m_numverts)
			{
				osd_printf_error("Error: vertnum (%d) plus poly vertex count (%d) > %d\n", vertnum, m_poly[polynum].get_vertcount(), m_numverts);
				fflush(stdout);
			}

			assert(vertnum + m_poly[polynum].get_vertcount() <= m_numverts);

			/*if (m_shaders->enabled() && d3dintf->post_fx_available)
			{
				m_shaders->render_quad(&m_poly[polynum], vertnum);
			}*/
			//else

			// add the primitives
			m_device_context->IASetPrimitiveTopology(m_poly[polynum].get_type());
			m_device_context->Draw(m_poly[polynum].get_vertcount(), vertnum);

			vertnum += m_poly[polynum].get_vertcount();
		}

		//m_shaders->end_draw();

		// reset the vertex count
		m_numverts = 0;
		m_numpolys = 0;
	}

    void renderer::set_fullscreen_gamma()
    {
        // only set the gamma if it's not 1.0f
        windows_options &options = downcast<windows_options &>(window().machine().options());
        float brightness = options.full_screen_brightness();
        float contrast = options.full_screen_contrast();
        float gamma = options.full_screen_gamma();
        if (brightness != 1.0f || contrast != 1.0f || gamma != 1.0f)
        {
            DXGI_GAMMA_CONTROL_CAPABILITIES gamma_caps = { 0 };

            // Make sure to get the output from the swap chain since it changes
            // when we enter full screen mode
            ComPtr<IDXGIOutput> fullscreen_output;
            ThrowIfFailed(m_swap_chain->GetContainingOutput(fullscreen_output.GetAddressOf()));

            HRESULT result = fullscreen_output->GetGammaControlCapabilities(&gamma_caps);

            // warn if we can't do it
            if (FAILED(result) || gamma_caps.NumGammaControlPoints == 0)
            {
                osd_printf_warning("Direct3D: Warning - GetGammaControlCapabilities failed. Error: 0x%X\n", (unsigned int)result);
            }
            else
            {
                // create a standard ramp and set it
                DXGI_GAMMA_CONTROL ramp;
                ramp.Scale.Red = ramp.Scale.Blue = ramp.Scale.Green = 1;
                ramp.Offset.Red = ramp.Offset.Blue = ramp.Offset.Green = 0;

                // get our range size
                float target_range = gamma_caps.MaxConvertedValue - gamma_caps.MinConvertedValue;

                for (int i = 0; i < gamma_caps.NumGammaControlPoints; i++)
                {
                    float source_value = (float)i / gamma_caps.NumGammaControlPoints;

                    float current_ramp = apply_brightness_contrast_gamma_fp(source_value, brightness, contrast, gamma);

                    float target_value = MIN((current_ramp * target_range) + gamma_caps.MinConvertedValue, gamma_caps.MaxConvertedValue);

                    // Account for floating point errors
                    // negative numbers should be max
                    if (target_value < 0)
                    {
                        target_value = gamma_caps.MaxConvertedValue;
                    }

                    // Compute the target as the percent of the range, adjusted by the minimum value
                    ramp.GammaCurve[i].Red = ramp.GammaCurve[i].Green = ramp.GammaCurve[i].Blue = (current_ramp * target_range) + gamma_caps.MinConvertedValue;
                }

                HRESULT result = fullscreen_output->SetGammaControl(&ramp);
                if (FAILED(result))
                {
                    osd_printf_warning("Direct3D11: Warning - Set gamma control failed with 0x%X.\n", (unsigned int)result);
                }
            }
        }
    }

	HRESULT renderer::create_dxgi_factory2_dynamic(UINT flags, REFIID riid, IDXGIFactory2 **ppFactory)
	{
		if (m_dxgi_module == NULL)
		{
			m_dxgi_module = LoadLibrary(_T("Dxgi.dll"));
			if (m_dxgi_module == NULL)
			{
				osd_printf_error("Failed to load Dxgi.dll. Error: 0x%x\n", (unsigned int)GetLastError());
				return E_FAIL;
			}
		}

		if (m_pfnCreateDXGIFactory2 == NULL)
		{
			m_pfnCreateDXGIFactory2 = (PFN_CREATEDXGIFACTORY2)GetProcAddress(m_dxgi_module, "CreateDXGIFactory2");
			if (m_pfnCreateDXGIFactory2 == NULL)
			{
				osd_printf_error("Could not find proc address for CreateDXGIFactory2. Error: 0x%x\n", (unsigned int)GetLastError());
				return 1;
			}
		}

		return m_pfnCreateDXGIFactory2(flags, riid, (LPVOID*)ppFactory);
	}

	HRESULT	renderer::create_d3d11_device_dynamic(
		IDXGIAdapter *				pAdapter,
		D3D_DRIVER_TYPE				DriverType,
		HMODULE						Software,
		UINT						Flags,
		const D3D_FEATURE_LEVEL *	pFeatureLevels,
		UINT						FeatureLevels,
		UINT						SDKVersion,
		ID3D11Device **				ppDevice,
		D3D_FEATURE_LEVEL*			pFeatureLevel,
		ID3D11DeviceContext **		ppImmediateContext
		)
	{
		if (m_d3d_module == NULL)
		{
			m_d3d_module = LoadLibrary(_T("D3D11.dll"));
			if (m_d3d_module == NULL)
			{
				osd_printf_error("Failed to load D3D11.dll. Error: 0x%x\n", (unsigned int)GetLastError());
				return E_FAIL;
			}
		}

		if (m_pfnD3D11CreateDevice == NULL)
		{
			m_pfnD3D11CreateDevice = (PFN_D3D11_CREATE_DEVICE)GetProcAddress(m_d3d_module, "D3D11CreateDevice");
			if (m_pfnD3D11CreateDevice == NULL)
			{
				osd_printf_error("Could not find proc address for D3D11CreateDevice. Error: 0x%x\n", (unsigned int)GetLastError());
				return 1;
			}
		}

		return m_pfnD3D11CreateDevice(
			pAdapter,
			DriverType,
			Software,
			Flags,
			pFeatureLevels,
			FeatureLevels,
			SDKVersion,
			ppDevice,
			pFeatureLevel,
			ppImmediateContext);
	}


#ifdef _DEBUG
	// Check whether the SDK layers are available
	bool renderer::SdkLayersAvailable()
	{
		HRESULT hr = create_d3d11_device_dynamic(
			nullptr,
			D3D_DRIVER_TYPE_NULL,       // There is no need to create a real hardware device.
			0,
			D3D11_CREATE_DEVICE_DEBUG,  // Check for the SDK layers.
			nullptr,                    // Any feature level will do.
			0,
			D3D11_SDK_VERSION,          // Always set this to D3D11_SDK_VERSION for Windows Store apps.
			nullptr,                    // No need to keep the D3D device reference.
			nullptr,                    // No need to know the feature level.
			nullptr                     // No need to keep the D3D device context reference.
			);

		return SUCCEEDED(hr);
	}
#endif

	float renderer::refresh_as_float(const DXGI_RATIONAL &refresh)
	{
		return refresh.Denominator
			? (float)refresh.Numerator / refresh.Denominator
			: 0.0f;
	}
}