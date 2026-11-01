includes("packages/w/wren/xmake.lua")
-- official package from my repo with custom settings
includes("packages/h/haclog/xmake.lua")

includes("xmake/config.lua")
includes("xmake/options.lua")
includes("xmake/requires.lua")
includes("xmake/flags.lua")
includes("xmake/tests.lua")

add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate", {outputdir = "."})

setup_options()
setup_global_flags()
setup_requires()

local embedded = is_embedded()
local dev_mode = is_dev_mode()
local suffix = embedded and "embedded" or ""

local plat = get_config("plat") or os.host()
local arch = get_config("arch") or os.arch()
local mode = get_config("mode") or "release"

target("GrnGame")
	if is_plat("android") then
		add_cflags("-fPIC")
	end

	add_defines("WITH_SDL3_STATIC")
	set_languages("c17")
	set_kind("static")

	add_files("grngame/**.c")
	-- wasm dosnt like double main declaration (segfault)
	remove_files("grngame/embedded/embedded_file_generator_main.c")

	add_headerfiles("grngame/**.h")
	add_includedirs(".", { public = true })

	apply_game_packages()

if dev_mode then
	if get_config("plat") == os.host() and get_config("arch") == os.arch() then
		target("Embedded-" .. plat .. "-" .. arch .. "-" .. mode)
			set_languages("c17")
			set_kind("binary")
			set_targetdir(path.join("$(builddir)", "Embedded"))
			add_files("grngame/embedded/embedded_file_generator_main.c")
			add_headerfiles("grngame/**.h")
			add_deps("GrnGame")
	end
end

if is_plat("android") then
	target("Runtime-" .. plat .. "-" .. arch .. "-" .. mode .. suffix)
		set_kind("shared")
		set_basename("GrnGame")
		set_languages("c17")
		set_targetdir(path.join("$(builddir)", "Runtime"))
		add_files("runtime/main.c")
		add_deps("GrnGame")
else
	target("Runtime-" .. plat .. "-" .. arch .. "-" .. mode .. suffix)
		set_kind("binary")
		set_languages("c17")
		set_targetdir(path.join("$(builddir)", "Runtime"))
		add_files("runtime/main.c")
		add_deps("GrnGame")
end


setup_tests(
	{ "json", "pad_event", "engine_work", "sound" ,"version","draw_text"}, -- target_tests
	{"engine_work"}                                               -- auto_tests
)