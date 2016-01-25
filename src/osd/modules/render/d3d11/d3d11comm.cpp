// license:BSD-3-Clause
// copyright-holders:Aaron Giles, Brad Hughes
//============================================================
//
//  d3d11comm.c - Win32 Direct3D 11 common
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

// MAME headers
#include "emu.h"
#include "render.h"

#include "rendutil.h"
#include "emuopts.h"
#include "aviio.h"

// MAMEOS headers
#include "..\d3d\d3dintf.h"
#include "winmain.h"
#include "window.h"
#include "..\d3d\d3dcomm.h"

#include "..\drawd3d11.h"
#include "d3d11comm.h"

namespace d3d11
{
	using namespace Microsoft::WRL;

	texture_manager::texture_manager(renderer *d3d)
	{
		m_renderer = d3d;

		m_texlist = NULL;
		m_vector_texture = NULL;
		m_default_texture = NULL;

        // Minimum capabilities for Direct3D level 9_1 requires D3DCAPS2_DYNAMICTEXTURES
        // So assume we support them
		m_dynamic_supported = true;
		osd_printf_verbose("Direct3D: Using dynamic textures\n");

        DWORD max_texture_dim = 0;
        switch (d3d->get_feature_level())
        {
        case D3D_FEATURE_LEVEL_9_1:
        case D3D_FEATURE_LEVEL_9_2:
            max_texture_dim = D3D_FL9_1_REQ_TEXTURE2D_U_OR_V_DIMENSION;
            break;

        case D3D_FEATURE_LEVEL_9_3:
            max_texture_dim = D3D_FL9_3_REQ_TEXTURE2D_U_OR_V_DIMENSION;
            break;

        case D3D_FEATURE_LEVEL_10_0:
        case D3D_FEATURE_LEVEL_10_1:
            max_texture_dim = D3D10_REQ_TEXTURE2D_U_OR_V_DIMENSION;
            break;

        case D3D_FEATURE_LEVEL_11_0:
        case D3D_FEATURE_LEVEL_11_1:
            max_texture_dim = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
            break;
        }

		m_texture_max_aspect = m_texture_max_height = m_texture_max_width = max_texture_dim;

		// pick a YUV texture format
		m_yuv_format = DXGI_FORMAT_YUY2;
        UINT support;
        HRESULT result = d3d->get_device()->CheckFormatSupport(DXGI_FORMAT_YUY2, &support);
        if (FAILED(result) || !(support & D3D11_FORMAT_SUPPORT_TEXTURE2D))
        {
            m_yuv_format = DXGI_FORMAT_B8G8R8A8_UNORM;
        }
        osd_printf_verbose("Direct3D: YUV format = %s\n", (m_yuv_format == DXGI_FORMAT_YUY2) ? "YUY2" : "RGB");

		// set the max texture size
		d3d->window().target()->set_max_texture_size(m_texture_max_width, m_texture_max_height);
		osd_printf_verbose("Direct3D: Max texture size = %dx%d\n", (int)m_texture_max_width, (int)m_texture_max_height);
	}

	texture_manager::~texture_manager()
	{
	}

	void texture_manager::create_resources()
	{
		// experimental: load a PNG to use for vector rendering; it is treated
		// as a brightness map
		emu_file file(m_renderer->window().machine().options().art_path(), OPEN_FLAG_READ);
		render_load_png(m_vector_bitmap, file, NULL, "vector.png");
		if (m_vector_bitmap.valid())
		{
			m_vector_bitmap.fill(rgb_t(0xff, 0xff, 0xff, 0xff));
			render_load_png(m_vector_bitmap, file, NULL, "vector.png", true);
		}

		m_default_bitmap.allocate(8, 8);
		m_default_bitmap.fill(rgb_t(0xff, 0xff, 0xff, 0xff));

		if (m_default_bitmap.valid())
		{
			render_texinfo texture;

			// fake in the basic data so it looks like it came from render.c
			texture.base = m_default_bitmap.raw_pixptr(0);
			texture.rowpixels = m_default_bitmap.rowpixels();
			texture.width = m_default_bitmap.width();
			texture.height = m_default_bitmap.height();
			texture.palette = NULL;
			texture.seqid = 0;

			// now create it
			m_default_texture = global_alloc(texture_info(this, &texture, m_renderer->window().prescale(), PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA) | PRIMFLAG_TEXFORMAT(TEXFORMAT_ARGB32)));
		}

		// experimental: if we have a vector bitmap, create a texture for it
		if (m_vector_bitmap.valid())
		{
			render_texinfo texture;

			// fake in the basic data so it looks like it came from render.c
			texture.base = &m_vector_bitmap.pix32(0);
			texture.rowpixels = m_vector_bitmap.rowpixels();
			texture.width = m_vector_bitmap.width();
			texture.height = m_vector_bitmap.height();
			texture.palette = NULL;
			texture.seqid = 0;

			// now create it
			m_vector_texture = global_alloc(texture_info(this, &texture, m_renderer->window().prescale(), PRIMFLAG_BLENDMODE(BLENDMODE_ALPHA) | PRIMFLAG_TEXFORMAT(TEXFORMAT_ARGB32)));
		}
	}

	void texture_manager::delete_resources()
	{
		// is part of m_texlist and will be free'd there
		//global_free(m_default_texture);
		m_default_texture = NULL;

		//global_free(m_vector_texture);
		m_vector_texture = NULL;

		// free all textures
		while (m_texlist != NULL)
		{
			texture_info *tex = m_texlist;
			m_texlist = tex->get_next();
			global_free(tex);
		}
	}

    UINT32 texture_manager::texture_compute_hash(const render_texinfo *texture, UINT32 flags)
    {
        return (FPTR)texture->base ^ (flags & (PRIMFLAG_BLENDMODE_MASK | PRIMFLAG_TEXFORMAT_MASK));
    }

    texture_info *texture_manager::find_texinfo(const render_texinfo *texinfo, UINT32 flags)
    {
        UINT32 hash = texture_compute_hash(texinfo, flags);
        texture_info *texture;

        // find a match
        for (texture = m_renderer->get_texture_manager()->get_texlist(); texture != NULL; texture = texture->get_next())
        {
            UINT32 test_screen = (UINT32)texture->get_texinfo().osddata >> 1;
            UINT32 test_page = (UINT32)texture->get_texinfo().osddata & 1;
            UINT32 prim_screen = (UINT32)texinfo->osddata >> 1;
            UINT32 prim_page = (UINT32)texinfo->osddata & 1;
            if (test_screen != prim_screen || test_page != prim_page)
            {
                continue;
            }

            if (texture->get_hash() == hash &&
                texture->get_texinfo().base == texinfo->base &&
                texture->get_texinfo().width == texinfo->width &&
                texture->get_texinfo().height == texinfo->height &&
                ((texture->get_flags() ^ flags) & (PRIMFLAG_BLENDMODE_MASK | PRIMFLAG_TEXFORMAT_MASK)) == 0)
            {
                return texture;
            }
        }

        return NULL;
    }

	void texture_manager::update_textures()
	{
		for (render_primitive *prim = m_renderer->window().m_primlist->first(); prim != NULL; prim = prim->next())
		{
			if (prim->texture.base != NULL)
			{
				texture_info *texture = find_texinfo(&prim->texture, prim->flags);
				if (texture == NULL)
				{
					// if there isn't one, create a new texture
					global_alloc(texture_info(this, &prim->texture, m_renderer->window().prescale(), prim->flags));
				}
				else
				{
					// if there is one, but with a different seqid, copy the data
					if (texture->get_texinfo().seqid != prim->texture.seqid)
					{
						texture->set_data(&prim->texture, prim->flags);
						texture->get_texinfo().seqid = prim->texture.seqid;
					}
				}
			}
			/*else if (m_renderer->get_shaders()->vector_enabled() && PRIMFLAG_GET_VECTORBUF(prim->flags))
			{
				if (!m_renderer->get_shaders()->get_vector_target())
				{
					m_renderer->get_shaders()->create_vector_target(prim);
				}
			}*/
		}
	}

	void poly_info::init(D3D11_PRIMITIVE_TOPOLOGY type, UINT32 count, UINT32 numverts,
		UINT32 flags, texture_info *texture, UINT32 modmode,
		float line_time, float line_length,
		float prim_width, float prim_height)
	{
		init(type, count, numverts, flags, texture, modmode, prim_width, prim_height);
		m_line_time = line_time;
		m_line_length = line_length;
	}

	void poly_info::init(D3D11_PRIMITIVE_TOPOLOGY type, UINT32 count, UINT32 numverts,
		UINT32 flags, texture_info *texture, UINT32 modmode,
		float prim_width, float prim_height)
	{
		m_type = type;
		m_count = count;
		m_numverts = numverts;
		m_flags = flags;
		m_texture = texture;
		m_modmode = modmode;
		m_prim_width = prim_width;
		m_prim_height = prim_height;
	}

	//============================================================
	//  texture_info destructor
	//============================================================

	texture_info::~texture_info()
	{
		m_d3dfinaltex.Reset();
		m_d3dtex.Reset();
		m_d3dsurface.Reset();
	}

	//============================================================
	//  texture_info constructor
	//============================================================

	texture_info::texture_info(texture_manager *manager, const render_texinfo* texsource, int prescale, UINT32 flags)
		: m_d3dtex(nullptr),
		  m_d3dsurface(nullptr),
          m_d3dfinaltex(nullptr)
	{
		HRESULT result;

		// fill in the core data
		m_texture_manager = manager;
		m_renderer = m_texture_manager->get_d3d();
		m_hash = m_texture_manager->texture_compute_hash(texsource, flags);
		m_flags = flags;
		m_texinfo = *texsource;
		m_xprescale = prescale;
		m_yprescale = prescale;

		// compute the size
		compute_size(texsource->width, texsource->height);

		// non-screen textures are easy
		if (!PRIMFLAG_GET_SCREENTEX(flags))
		{
			assert(PRIMFLAG_TEXFORMAT(flags) != TEXFORMAT_YUY16);
			D3D11_TEXTURE2D_DESC texdesc = { 0 };
			texdesc.Width = m_rawdims.c.x;
			texdesc.Height = m_rawdims.c.y;
			texdesc.MipLevels = 1;
			texdesc.ArraySize = 1;
			texdesc.SampleDesc.Count = 1;
			texdesc.SampleDesc.Quality = 0;
			texdesc.Usage = D3D11_USAGE_DYNAMIC;
			texdesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			texdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
			texdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

			result = m_renderer->get_device()->CreateTexture2D(&texdesc, nullptr, m_d3dtex.GetAddressOf());
			NAME_OBJECT(m_d3dtex, "Non-screen 2D Texture");
			if (FAILED(result))
			{
				goto error;
			}

			m_d3dfinaltex = m_d3dtex;
			m_type = TEXTURE_TYPE_PLAIN;
		}

		// screen textures are allocated differently
		else
		{
			DXGI_FORMAT format;
			D3D11_USAGE usage = m_texture_manager->is_dynamic_supported() ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;

			int maxdim = MAX(m_renderer->get_presentation()->Width, m_renderer->get_presentation()->Height);

			// pick the format
			if (PRIMFLAG_GET_TEXFORMAT(flags) == TEXFORMAT_YUY16)
			{
				format = m_texture_manager->get_yuv_format();
			}
			else if (PRIMFLAG_GET_TEXFORMAT(flags) == TEXFORMAT_ARGB32 || PRIMFLAG_GET_TEXFORMAT(flags) == TEXFORMAT_PALETTEA16)
			{
				format = DXGI_FORMAT_B8G8R8A8_UNORM;
			}
			else
			{
				format = m_renderer->get_screen_format();
			}

			// don't prescale above screen size
			while (m_xprescale > 1 && m_rawdims.c.x * m_xprescale >= 2 * maxdim)
			{
				m_xprescale--;
			}
			while (m_xprescale > 1 && m_rawdims.c.x * m_xprescale > manager->get_max_texture_width())
			{
				m_xprescale--;
			}
			while (m_yprescale > 1 && m_rawdims.c.y * m_yprescale >= 2 * maxdim)
			{
				m_yprescale--;
			}
			while (m_yprescale > 1 && m_rawdims.c.y * m_yprescale > manager->get_max_texture_height())
			{
				m_yprescale--;
			}

			int prescale = m_renderer->window().prescale();
			if (m_xprescale != prescale || m_yprescale != prescale)
			{
				osd_printf_verbose("Direct3D: adjusting prescale from %dx%d to %dx%d\n", prescale, prescale, m_xprescale, m_yprescale);
			}

			// loop until we allocate something or error
			for (int attempt = 0; attempt < 2; attempt++)
			{
				// second attempt is always 1:1
				if (attempt == 1)
				{
					m_xprescale = m_yprescale = 1;
				}

				// screen textures with no prescaling are pretty easy
				if (m_xprescale == 1 && m_yprescale == 1)
				{
					D3D11_TEXTURE2D_DESC texdesc = { 0 };
					texdesc.Width = m_rawdims.c.x;
					texdesc.Height = m_rawdims.c.y;
					texdesc.MipLevels = 1;
					texdesc.ArraySize = 1;
					texdesc.SampleDesc.Count = 1;
					texdesc.SampleDesc.Quality = 0;
					texdesc.Usage = usage;
					texdesc.Format = format;
					texdesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
					texdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

					result = m_renderer->get_device()->CreateTexture2D(&texdesc, nullptr, m_d3dtex.GetAddressOf());
					NAME_OBJECT(m_d3dtex, "Screen 2D Texture");
					if (FAILED(result))
					{
						goto error;
					}

					if (result == S_OK)
					{
						m_d3dfinaltex = m_d3dtex;
						m_type = m_texture_manager->is_dynamic_supported() ? TEXTURE_TYPE_DYNAMIC : TEXTURE_TYPE_PLAIN;

						break;
					}
				}

				// screen textures with prescaling require two allocations
				else
				{
					// we allocate a dynamic texture for the source
					D3D11_TEXTURE2D_DESC texdesc1 = { 0 };
					texdesc1.Width = m_rawdims.c.x;
					texdesc1.Height = m_rawdims.c.y;
					texdesc1.MipLevels = 1;
					texdesc1.ArraySize = 1;
					texdesc1.SampleDesc.Count = 1;
					texdesc1.SampleDesc.Quality = 0;
					texdesc1.Usage = usage;
					texdesc1.Format = format;
					texdesc1.BindFlags = D3D11_BIND_SHADER_RESOURCE;
					texdesc1.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

					result = m_renderer->get_device()->CreateTexture2D(&texdesc1, nullptr, m_d3dtex.GetAddressOf());
					NAME_OBJECT(m_d3dtex, "2D Texture with prescale 1");
					if (result != S_OK)
						continue;

					m_type = m_texture_manager->is_dynamic_supported() ? TEXTURE_TYPE_DYNAMIC : TEXTURE_TYPE_PLAIN;

					// for the target surface, we allocate a render target texture
					int scwidth = m_rawdims.c.x * m_xprescale;
					int scheight = m_rawdims.c.y * m_yprescale;

					// target surfaces typically cannot be YCbCr, so we always pick RGB in that case
					DXGI_FORMAT finalfmt = (format != m_texture_manager->get_yuv_format()) ? format : DXGI_FORMAT_B8G8R8A8_UNORM;
					D3D11_TEXTURE2D_DESC texdesc2 = { 0 };
					texdesc2.Width = scwidth;
					texdesc2.Height = scheight;
					texdesc2.MipLevels = 1;
					texdesc2.ArraySize = 1;
					texdesc2.SampleDesc.Count = 1;
					texdesc2.SampleDesc.Quality = 0;
					texdesc2.Usage = D3D11_USAGE_DYNAMIC;
					texdesc2.Format = finalfmt;
					texdesc2.BindFlags = D3D11_BIND_RENDER_TARGET;
					texdesc2.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

					result = m_renderer->get_device()->CreateTexture2D(&texdesc2, nullptr, m_d3dfinaltex.GetAddressOf());
					NAME_OBJECT(m_d3dfinaltex, "2D Texture with prescale 2");
					if (result == S_OK)
					{
						break;
					}

					m_d3dtex.Reset();
				}
			}
		}

		// copy the data to the texture
		set_data(texsource, flags);

		//texsource->osdhandle = (void*)this;
		// add us to the texture list
		if (m_texture_manager->get_texlist() != NULL)
			m_texture_manager->get_texlist()->m_prev = this;
		m_prev = NULL;
		m_next = m_texture_manager->get_texlist();
		m_texture_manager->set_texlist(this);
		return;

	error:

		osd_printf_error("Direct3D: Critical warning: A texture failed to allocate. Expect things to get bad quickly.\n");
        if (m_d3dsurface != nullptr)
            m_d3dsurface.Reset();
        if (m_d3dtex != nullptr)
            m_d3dtex.Reset();
	}

	//============================================================
	//  texture_info::compute_size_subroutine
	//============================================================

	void texture_info::compute_size_subroutine(int texwidth, int texheight, int* p_width, int* p_height)
	{
		int finalheight = texheight;
		int finalwidth = texwidth;

		// adjust the aspect ratio if we need to
		while (finalwidth < finalheight && finalheight / finalwidth > m_texture_manager->get_max_texture_aspect())
		{
			finalwidth *= 2;
		}
		while (finalheight < finalwidth && finalwidth / finalheight > m_texture_manager->get_max_texture_aspect())
		{
			finalheight *= 2;
		}

		*p_width = finalwidth;
		*p_height = finalheight;
	}


	//============================================================
	//  texture_info::compute_size
	//============================================================

	void texture_info::compute_size(int texwidth, int texheight)
	{
		int finalheight = texheight;
		int finalwidth = texwidth;

		m_xborderpix = 0;
		m_yborderpix = 0;

		// if we're not wrapping, add a 1-2 pixel border on all sides
		if (ENABLE_BORDER_PIX && !(m_flags & PRIMFLAG_TEXWRAP_MASK))
		{
			// note we need 2 pixels in X for YUY textures
			m_xborderpix = (PRIMFLAG_GET_TEXFORMAT(m_flags) == TEXFORMAT_YUY16) ? 2 : 1;
			m_yborderpix = 1;
		}

		// compute final texture size
		finalwidth += 2 * m_xborderpix;
		finalheight += 2 * m_yborderpix;

		compute_size_subroutine(finalwidth, finalheight, &finalwidth, &finalheight);

		// if we added pixels for the border, and that just barely pushed us over, take it back
		if (finalwidth > m_texture_manager->get_max_texture_width() || finalheight > m_texture_manager->get_max_texture_height())
		{
			finalheight = texheight;
			finalwidth = texwidth;

			m_xborderpix = 0;
			m_yborderpix = 0;

			compute_size_subroutine(finalwidth, finalheight, &finalwidth, &finalheight);
		}

		// if we're above the max width/height, do what?
		if (finalwidth > m_texture_manager->get_max_texture_width() || finalheight > m_texture_manager->get_max_texture_height())
		{
			static int printed = FALSE;
			if (!printed) osd_printf_warning("Texture too big! (wanted: %dx%d, max is %dx%d)\n", finalwidth, finalheight, (int)m_texture_manager->get_max_texture_width(), (int)m_texture_manager->get_max_texture_height());
			printed = TRUE;
		}

		// compute the U/V scale factors
		m_start.c.x = (float)m_xborderpix / (float)finalwidth;
		m_start.c.y = (float)m_yborderpix / (float)finalheight;
		m_stop.c.x = (float)(texwidth + m_xborderpix) / (float)finalwidth;
		m_stop.c.y = (float)(texheight + m_yborderpix) / (float)finalheight;

		// set the final values
		m_rawdims.c.x = finalwidth;
		m_rawdims.c.y = finalheight;
	}

	//============================================================
	//  texture_set_data
	//============================================================

	void texture_info::set_data(const render_texinfo *texsource, UINT32 flags)
	{
        D3D11_MAPPED_SUBRESOURCE rect;
		HRESULT result;

		// lock the texture
		// (map the texture)
		result = m_renderer->get_context()->Map(m_d3dtex.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &rect);
        if (result != S_OK)
        {
            return;
        }

		// loop over Y
		int miny = 0 - m_yborderpix;
		int maxy = texsource->height + m_yborderpix;
		for (int dsty = miny; dsty < maxy; dsty++)
		{
			int srcy = (dsty < 0) ? 0 : (dsty >= texsource->height) ? texsource->height - 1 : dsty;
			void *dst = (BYTE *)rect.pData + (dsty + m_yborderpix) * rect.RowPitch;

			// switch off of the format and
			switch (PRIMFLAG_GET_TEXFORMAT(flags))
			{
			case TEXFORMAT_PALETTE16:
				d3d::copyline_palette16((UINT32 *)dst, (UINT16 *)texsource->base + srcy * texsource->rowpixels, texsource->width, texsource->palette, m_xborderpix);
				break;

			case TEXFORMAT_PALETTEA16:
                d3d::copyline_palettea16((UINT32 *)dst, (UINT16 *)texsource->base + srcy * texsource->rowpixels, texsource->width, texsource->palette, m_xborderpix);
				break;

			case TEXFORMAT_RGB32:
                d3d::copyline_rgb32((UINT32 *)dst, (UINT32 *)texsource->base + srcy * texsource->rowpixels, texsource->width, texsource->palette, m_xborderpix);
				break;

			case TEXFORMAT_ARGB32:
                d3d::copyline_argb32((UINT32 *)dst, (UINT32 *)texsource->base + srcy * texsource->rowpixels, texsource->width, texsource->palette, m_xborderpix);
				break;

			case TEXFORMAT_YUY16:
				if (m_texture_manager->get_yuv_format() == DXGI_FORMAT_YUY2)
                    d3d::copyline_yuy16_to_yuy2((UINT16 *)dst, (UINT16 *)texsource->base + srcy * texsource->rowpixels, texsource->width, texsource->palette, m_xborderpix);
				else
                    d3d::copyline_yuy16_to_argb((UINT32 *)dst, (UINT16 *)texsource->base + srcy * texsource->rowpixels, texsource->width, texsource->palette, m_xborderpix);
				break;

			default:
				osd_printf_error("Unknown texture blendmode=%d format=%d\n", PRIMFLAG_GET_BLENDMODE(flags), PRIMFLAG_GET_TEXFORMAT(flags));
				break;
			}
		}

		// unlock
		m_renderer->get_context()->Unmap(m_d3dtex.Get(), 0);

		// prescale
		prescale();
	}

	//============================================================
	//  texture_info::prescale
	//============================================================

	void texture_info::prescale()
	{
		// if we don't need to, just skip it
		if (m_d3dtex == m_d3dfinaltex)
			return;

		assert(m_d3dsurface != nullptr);

		// set the source bounds
		D3D11_BOX source;
		source.left = source.top = 0;
		source.right = m_texinfo.width + 2 * m_xborderpix;
		source.bottom = m_texinfo.height + 2 * m_yborderpix;

		// set the target bounds
		D3D11_BOX dest;
		dest = source;
		dest.right *= m_xprescale;
		dest.bottom *= m_yprescale;

		// do the stretchrect
		m_renderer->get_context()->CopySubresourceRegion(m_d3dtex.Get(), 0, dest.right, dest.bottom, 0, m_d3dfinaltex.Get(), 0, &source);
	}
}