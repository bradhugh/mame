-- license:BSD-3-Clause
-- copyright-holders:MAMEdev Team

---------------------------------------------------------------------------
--
--   winrt.lua
--
--   Rules for the building of winrt OSD
--
---------------------------------------------------------------------------

dofile("modules.lua")

function maintargetosdoptions(_target,_subtarget)
end

project ("osd_" .. _OPTIONS["osd"])
	uuid (os.uuid("osd_" .. _OPTIONS["osd"]))
	kind (LIBTYPE)

	removeflags {
		"SingleOutputDir",
	}
	
	dofile("winrt_cfg.lua")
	osdmodulesbuild()
	
	includedirs {
		MAME_DIR .. "src/emu",
		MAME_DIR .. "src/osd",
		MAME_DIR .. "src/lib",
		MAME_DIR .. "src/lib/util",
		MAME_DIR .. "src/osd/modules/render",
		MAME_DIR .. "3rdparty",
	}

	includedirs {
		MAME_DIR .. "src/osd/winrt",
	}

	files {
		MAME_DIR .. "src/osd/winrt/winrtcompat.cpp",
		MAME_DIR .. "src/osd/winrt/winrtcompat.h",
		MAME_DIR .. "src/osd/winrt/winrtmain.cpp",
		MAME_DIR .. "src/osd/winrt/winrtmain.h",
		MAME_DIR .. "src/osd/osdepend.h",
	}
	
project ("ocore_" .. _OPTIONS["osd"])
	uuid (os.uuid("ocore_" .. _OPTIONS["osd"]))
	kind (LIBTYPE)

	removeflags {
		"SingleOutputDir",	
	}

	dofile("winrt_cfg.lua")
	
	includedirs {
		MAME_DIR .. "src/emu",
		MAME_DIR .. "src/osd",
		MAME_DIR .. "src/lib",
		MAME_DIR .. "src/lib/util",
	}
	
	files {
		MAME_DIR .. "src/osd/osdnet.cpp",
		MAME_DIR .. "src/osd/osdnet.h",
		MAME_DIR .. "src/osd/osdcore.cpp",
		MAME_DIR .. "src/osd/osdcore.h",
		MAME_DIR .. "src/osd/modules/osdmodule.cpp",
		MAME_DIR .. "src/osd/modules/osdmodule.h",
--		MAME_DIR .. "src/osd/winrt/winrtdir.cpp",
--		MAME_DIR .. "src/osd/winrt/winrtfile.cpp",
--		MAME_DIR .. "src/osd/winrt/winrtmisc.cpp",
--		MAME_DIR .. "src/osd/winrt/winrttime.cpp",
		MAME_DIR .. "src/osd/modules/sync/osdsync.cpp",
		MAME_DIR .. "src/osd/modules/sync/osdsync.h",
		MAME_DIR .. "src/osd/modules/sync/work_osd.cpp",
	}
