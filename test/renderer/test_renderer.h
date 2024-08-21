
#include "taisei.h"

#include "renderer/api.h"
#include "events.h"
#include "log.h"
#include "util/compat.h"
#include "video.h"
#include "config.h"

#include <locale.h>

static void test_init_log(void) {
	log_init(LOG_ALL);
	log_add_output(LOG_ALL, SDL_RWFromFP(stderr, false), log_formatter_console);
}

static void test_init_sdl(void) {
	mem_install_sdl_callbacks();

	if(SDL_Init(SDL_INIT_EVENTS) < 0) {
		log_fatal("SDL_Init() failed: %s", SDL_GetError());
	}

	events_init();
}

static void test_init_basic(void) {
	setlocale(LC_ALL, "C");
	thread_init();
	test_init_log();
	test_init_sdl();
}

static void test_shutdown_basic(void) {
	log_shutdown();
	events_shutdown();
	thread_shutdown();
	SDL_Quit();
}

static ShaderObject *test_renderer_load_glsl(ShaderStage stage, const char *src) {
	// TODO: This is mostly copypasted from resource/shader_object; add a generic API for this

	ShaderSource s = {
		.content = (char*)src,
		.content_size = strlen(src),
		.stage = stage,
		.lang = {
			.lang = SHLANG_GLSL,
			.glsl.version = { 330, GLSL_PROFILE_CORE }
		},
	};

	ShaderLangInfo altlang = { SHLANG_INVALID };

	if(!r_shader_language_supported(&s.lang, &altlang)) {
		if(altlang.lang == SHLANG_INVALID) {
			log_fatal("Shading language not supported by backend");
		}

		log_warn("Shading language not supported by backend, attempting to translate");
		assert(r_shader_language_supported(&altlang, NULL));

		spirv_init_compiler();

		ShaderSource newsrc;
		bool result = spirv_transpile(&s, &newsrc, &(SPIRVTranspileOptions) {
			.lang = &altlang,
			.optimization_level = SPIRV_OPTIMIZE_NONE,
			.filename = "<embedded>",
		});

		if(!result) {
			log_fatal("Shader translation failed");
		}

		s = newsrc;
	}

	ShaderObject *obj = r_shader_object_compile(&s);

	if(!obj) {
		log_fatal("Failed to compile shader");
	}

	if(s.content != src) {
		shader_free_source(&s);
	}

	return obj;
}

static void test_init_renderer(void) {
	test_init_basic();

	config_set_int(CONFIG_VSYNC, 1);
	config_set_int(CONFIG_VID_RESIZABLE, 1);

	video_init(&(VideoInitParams) {
		.width = 800,
		.height = 600,
	});
}

static void test_shutdown_renderer(void) {
	video_shutdown();
	test_shutdown_basic();
}
