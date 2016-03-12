// license:BSD-3-Clause
// copyright-holders:Ryan Holtz
//============================================================
//
//  effectmanager.cpp - BGFX shader effect manager
//
//  Maintains a string-to-entry lookup of BGFX shader
//  effects, defined by effect.h and read by effectreader.h
//
//============================================================

#include "emu.h"

#include <rapidjson/document.h>

#include <bgfx/bgfxplatform.h>
#include <bgfx/bgfx.h>
#include <bx/readerwriter.h>

#include "effectmanager.h"
#include "effectreader.h"
#include "effect.h"

#ifndef BGFX_EFFECTS_PATH
#define BGFX_EFFECTS_PATH "bgfx/effects/"
#endif

using namespace rapidjson;

effect_manager::~effect_manager()
{
	for (std::pair<std::string, bgfx_effect*> effect : m_effects)
	{
		delete effect.second;
	}
	m_effects.clear();
}

bgfx_effect* effect_manager::effect(std::string name)
{
	std::map<std::string, bgfx_effect*>::iterator iter = m_effects.find(name);
	if (iter != m_effects.end())
	{
		return iter->second;
	}

	return load_effect(name);
}

bgfx_effect* effect_manager::load_effect(std::string name) {
	std::string path = BGFX_EFFECTS_PATH + name + ".json";

	bx::CrtFileReader reader;
	bx::open(&reader, path.c_str());

	int32_t size = (uint32_t)bx::getSize(&reader);

	char* data = new char[size + 1];
	bx::read(&reader, reinterpret_cast<void*>(data), size);
	bx::close(&reader);
	data[size] = 0;

	Document document;
	document.Parse<0>(data);
	bgfx_effect* effect = effect_reader::read_from_value(m_shaders, document);

	m_effects[name] = effect;

	return effect;
}
