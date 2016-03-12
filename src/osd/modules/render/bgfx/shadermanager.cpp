// license:BSD-3-Clause
// copyright-holders:Ryan Holtz
//============================================================
//
//  shadermanager.cpp - BGFX shader manager
//
//  Maintains a mapping between strings and BGFX shaders,
//  either vertex or pixel/fragment.
//
//============================================================

#include "emu.h"

#include <bgfx/bgfxplatform.h>
#include <bgfx/bgfx.h>
#include <bx/fpumath.h>
#include <bx/readerwriter.h>

#include "shadermanager.h"

#ifndef BGFX_DX9_SHADER_ROOT
#define BGFX_DX9_SHADER_ROOT "shaders/dx9/"
#endif

#ifndef BGFX_DX11_SHADER_ROOT
#define BGFX_DX11_SHADER_ROOT "shaders/dx11/"
#endif

#ifndef BGFX_OPENGL_SHADER_ROOT
#define BGFX_OPENGL_SHADER_ROOT "shaders/glsl/"
#endif

#ifndef BGFX_METAL_SHADER_ROOT
#define BGFX_METAL_SHADER_ROOT "shaders/metal/"
#endif

#ifndef BGFX_OPENGLES_SHADER_ROOT
#define BGFX_OPENGLES_SHADER_ROOT "shaders/gles/"
#endif

shader_manager::~shader_manager()
{
	for (std::pair<std::string, bgfx::ShaderHandle> shader : m_shaders)
	{
		bgfx::destroyShader(shader.second);
	}
	m_shaders.clear();
}

bgfx::ShaderHandle shader_manager::shader(std::string name)
{
	std::map<std::string, bgfx::ShaderHandle>::iterator iter = m_shaders.find(name);
	if (iter != m_shaders.end())
	{
		return iter->second;
	}

	return load_shader(name);
}

bgfx::ShaderHandle shader_manager::load_shader(std::string name) {
	std::string shader_path = BGFX_DX9_SHADER_ROOT;
	switch (bgfx::getRendererType())
	{
	case bgfx::RendererType::Direct3D11:
	case bgfx::RendererType::Direct3D12:
		shader_path = BGFX_DX11_SHADER_ROOT;
		break;

	case bgfx::RendererType::OpenGL:
		shader_path = BGFX_OPENGL_SHADER_ROOT;
		break;

	case bgfx::RendererType::Metal:
		shader_path = BGFX_METAL_SHADER_ROOT;
		break;

	case bgfx::RendererType::OpenGLES:
		shader_path = BGFX_OPENGLES_SHADER_ROOT;
		break;

	default:
		break;
	}

	bgfx::ShaderHandle handle = bgfx::createShader(load_mem(shader_path + name + ".bin"));

	m_shaders[name] = handle;

	return handle;
}

const bgfx::Memory* shader_manager::load_mem(std::string name) {
	bx::CrtFileReader reader;
	bx::open(&reader, name.c_str());

	uint32_t size = (uint32_t)bx::getSize(&reader);
	const bgfx::Memory* mem = bgfx::alloc(size + 1);
	bx::read(&reader, mem->data, size);
	bx::close(&reader);

	mem->data[mem->size - 1] = '\0';
	return mem;
}
