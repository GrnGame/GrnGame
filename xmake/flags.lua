function setup_global_flags()
	if is_plat("windows") then
		set_runtimes(is_mode("debug") and "MTd" or "MT")
	end

	if is_plat("android") then
		add_ldflags("-Wl,--export-dynamic", "-rdynamic", { force = true })
		add_ldflags("-u", "JNI_OnLoad", "-u", "SDL_main", { force = true })
		-- log in android
		add_syslinks("log")
		add_cflags("-fPIC")
	end

	if is_plat("wasm") then
		add_defines("GRNGAME_WASM", { public = true })
		add_ldflags(
			-- "--shell-file",
			-- "grngame/web/shell.html",
			"-sFORCE_FILESYSTEM=1",
			"-sASYNCIFY",
			"-sINITIAL_MEMORY=512MB",
			"-sALLOW_MEMORY_GROWTH=1",
			{ public = true, force = true }
		)
		if has_config("wasm_threads") then
			add_ldflags(
				"-sPTHREAD_POOL_SIZE=navigator.hardwareConcurrency",
				"-pthread",
				{ public = true, force = true }
			)
		else
			add_defines("NO_THREADS_WASM", { public = true })
		end
	end

	if is_plat("linux") then
		add_cxxflags("-frtti", "-fexceptions")
	end

	if is_arch("x64") then
		add_defines("GRNGAME_X64")
	end

	if has_config("software_renderer") then
		add_defines("GRNGAME_SOFTWARE", { public = true })
	end

	if has_config("pixel_int_position") then
		add_defines("PIXEL_INT_ONLY", { public = true })
	end

	if has_config("big_buffer") then
		add_defines("BUFFER_BIG_MEMORY_SPACE", { public = true })
	end

	set_warnings("all", "extra")

	if is_dev_mode() then
		add_defines("GRNGAME_DEV_MODE", { public = true })
		add_defines("GRNGAME_HOT_RELOAD_ENABLE")
	end

	if is_desktop_plat() then
		add_defines("GRNGAME_DESKTOP", { public = true })
	end

	if has_config("tracy") then
		add_defines("TRACY_ENABLE", { public = true })
	end

	-- platform defines
	if is_plat("linux") then
		add_defines("GRNGAME_LINUX", "_GNU_SOURCE", { public = true })
	elseif is_plat("windows") then
		add_defines("GRNGAME_WINDOWS", { public = true })
	elseif is_plat("macosx") then
		add_defines("GRNGAME_MACOS", { public = true })
	elseif is_plat("android") then
		add_defines("GRNGAME_ANDROID", { public = true })
	elseif is_plat("iphoneos") then
		add_defines("GRNGAME_IOS", { public = true })
	end

	-- mode defines
	if is_mode("debug") then
		add_defines("GRNGAME_DEBUG", { public = true })
	elseif is_mode("release") then
		add_defines("GRNGAME_RELEASE", { public = true })
		if not is_plat("macosx") then
			--set_policy("build.optimization.lto", true) temporary for speed build
		end
	end

	if is_embedded() then
		add_defines("GRNGAME_EMBED_ASSETS", { public = true })
	end

	add_defines("CGLM_USE_ANONYMOUS_STRUCT=1", { public = true })
end
