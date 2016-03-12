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

project ("qtdbg_" .. _OPTIONS["osd"])
	uuid (os.uuid("qtdbg_" .. _OPTIONS["osd"]))
	kind (LIBTYPE)

	dofile("winrt_cfg.lua")
	includedirs {
		MAME_DIR .. "src/emu",
		MAME_DIR .. "src/devices", -- accessing imagedev from debugger
		MAME_DIR .. "src/osd",
		MAME_DIR .. "src/lib",
		MAME_DIR .. "src/lib/util",
		MAME_DIR .. "src/osd/modules/render",
		MAME_DIR .. "3rdparty",
	}
	removeflags {
		"SingleOutputDir",
	}

	files {
		MAME_DIR .. "src/osd/modules/debugger/debugqt.cpp",
	}

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
		MAME_DIR .. "src/osd/windows",
	}

	files {
		MAME_DIR .. "src/osd/modules/osdwindow.cpp",
		MAME_DIR .. "src/osd/modules/osdwindow.h",
		MAME_DIR .. "src/osd/modules/render/drawnone.cpp",
		MAME_DIR .. "src/osd/modules/render/drawnone.h",
		MAME_DIR .. "src/osd/winrt/video.h",
		MAME_DIR .. "src/osd/winrt/video.cpp",
		MAME_DIR .. "src/osd/winrt/window.h",
		MAME_DIR .. "src/osd/winrt/window.cpp",
		MAME_DIR .. "src/osd/winrt/winrtcompat.cpp",
		MAME_DIR .. "src/osd/winrt/winrtcompat.h",
		MAME_DIR .. "src/osd/winrt/winrtcrtcompat.h",
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
		MAME_DIR .. "src/osd/strconv.cpp",
		MAME_DIR .. "src/osd/strconv.h",
		MAME_DIR .. "src/osd/winrt/winrtdir.cpp",
		MAME_DIR .. "src/osd/winrt/winrtfile.cpp",
		MAME_DIR .. "src/osd/winrt/winrtfile.h",
		MAME_DIR .. "src/osd/winrt/winrtclip.cpp",
		MAME_DIR .. "src/osd/winrt/winrtsocket.cpp",
		MAME_DIR .. "src/osd/winrt/winrtptty.cpp",
		MAME_DIR .. "src/osd/windows/winutil.cpp",
		MAME_DIR .. "src/osd/windows/winutil.h",
		MAME_DIR .. "src/osd/modules/sync/osdsync.cpp",
		MAME_DIR .. "src/osd/modules/sync/osdsync.h",
		MAME_DIR .. "src/osd/modules/sync/work_osd.cpp",
		MAME_DIR .. "src/osd/modules/lib/osdlib_winrt.cpp",
	}
