/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "test_common.h"

#include "renderer/common/shaderlib/shaderlib.h"
#include "util.h"
#include "util/stringops.h"
#include "vfs/setup.h"

static struct {
	MemArena arena;
} G;

static void on_vfs_up(CallChainResult ccr);

int main(int argc, char **argv) {
	test_init_basic();
	vfs_setup(CALLCHAIN(on_vfs_up, NULL));

	return 0;
}

static SDL_IOStream *glsl_open_callback(const char *path, void *userdata) {
	return vfs_open(path, VFS_MODE_READ);
}

static bool test_shader(const char *path) {
	ShaderStage stage;

	if(strendswith(path, ".vert.glsl")) {
		stage = SHADER_STAGE_VERTEX;
	} else if(strendswith(path, ".frag.glsl")) {
		stage = SHADER_STAGE_FRAGMENT;
	} else {
		log_warn("%s: Can't determine shader stage", path);
		return false;
	}

	ShaderMacro macros[] = {
		{ "BACKEND_SDLGPU", "1" },
		{},
	};

	GLSLSourceOptions opts = {
		.version = { 330, GLSL_PROFILE_CORE },
		.stage = stage,
		.macros = macros,
		.file_open_callback = glsl_open_callback,
	};

	ShaderSource src = {};

	if(!glsl_load_source(path, &src, &G.arena, &opts)) {
		log_error("%s: Failed to load source", path);
		return false;
	}

	// log_info("%s:\n%s", path, src.content);
	log_debug("%s: load OK", path);

	SPIRVTranspileOptions transpile_opts = {
		.compile = {
			.target = SPIRV_TARGET_VULKAN_10,
			.optimization_level = SPIRV_OPTIMIZE_NONE,
			.filename = path,
		},
		.decompile = {
			.reflect = true,
		},
	};

	struct {
		const char *name;
		ShaderLangInfo lang_info;
	} langs[] = {
		{ "SPIR-V", {
			.lang = SHLANG_SPIRV,
			.spirv.target = SPIRV_TARGET_VULKAN_10,
		}},
		{ "HLSL", {
			.lang = SHLANG_HLSL,
			.hlsl.shader_model = 51,
		}},
		{ "MSL", {
			.lang = SHLANG_MSL,
		}},
	};

	for(uint i = 0; i < ARRAY_SIZE(langs); ++i) {
		auto lang = langs + i;
		transpile_opts.decompile.lang = &lang->lang_info;

		log_debug("%s: testing %s", path, lang->name);
		log_sync(true);

		ShaderSource transpiled;
		if(!spirv_transpile(&src, &transpiled, &G.arena, &transpile_opts)) {
			log_fatal("%s: Transpile to %s failed", path, lang->name);
		}
	}

	return true;
}

static void *walker(const char *path, void *unused) {
	if(strendswith(path, ".glsl")) {
		auto snapshot = marena_snapshot(&G.arena);
		test_shader(path);
		marena_rollback(&G.arena, &snapshot);
	}

	return NULL;
}

static void on_vfs_up(CallChainResult ccr) {
	vfs_unmount("cache");
	marena_init(&G.arena, 0);
	spirv_init_compiler();
	vfs_dir_walk("res/shader", walker, NULL);
	marena_deinit(&G.arena);
	test_shutdown_basic();
}
