
#include "taisei.h"

#include "config.h"
#include "events.h"
#include "log.h"
#include "memory/scratch.h"
#include "renderer/api.h"
#include "resource/iqm_loader/iqm_loader.h"
#include "rwops/rwops_stdiofp.h"
#include "util/compat.h"
#include "video.h"

#include <locale.h>

static void test_init_log(void) {
	log_init(LOG_ALL);
	log_add_output(LOG_ALL, SDL_RWFromFP(stderr, false), log_formatter_console);
}

static void test_init_sdl(void) {
	mem_install_sdl_callbacks();

	if(!SDL_Init(SDL_INIT_EVENTS)) {
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
		.content = src,
		.content_size = strlen(src) + 1,
		.stage = stage,
		.lang = {
			.lang = SHLANG_GLSL,
			.glsl.version = { 330, GLSL_PROFILE_CORE }
		},
	};

	auto scratch = acquire_scratch_arena();

	SPIRVTranspileOptions transpile_opts = {
		.compile = {
			.optimization_level = SPIRV_OPTIMIZE_NONE,
			.filename = "<embedded>",
		},
	};

	if(!r_shader_language_supported(&s.lang, &transpile_opts)) {
		if(!transpile_opts.decompile.lang) {
			log_fatal("Shading language not supported by backend");
		}

		log_warn("Shading language not supported by backend, attempting to translate");
		assert(r_shader_language_supported(transpile_opts.decompile.lang, NULL));

		spirv_init_compiler();

		ShaderSource newsrc;
		bool result = spirv_transpile(&s, &newsrc, scratch, &transpile_opts);

		if(!result) {
			log_fatal("Shader translation failed");
		}

		s = newsrc;
	}

	ShaderObject *obj = r_shader_object_compile(&s);

	if(!obj) {
		log_fatal("Failed to compile shader");
	}

	release_scratch_arena(scratch);
	return obj;
}

attr_unused
static Texture *test_renderer_load_texture(const char *path) {
	auto istream = SDL_IOFromFile(path, "rb");

	if(!istream) {
		log_sdl_error(LOG_FATAL, "SDL_IOFromFile");
	}

	Pixmap px = {};
	PixmapFormat fmt = PIXMAP_FORMAT_RGBA8;
	PixmapOrigin origin = PIXMAP_ORIGIN_TOPLEFT;

	if(r_features() & RFEAT_TEXTURE_BOTTOMLEFT_ORIGIN) {
		origin = PIXMAP_ORIGIN_BOTTOMLEFT;
	}

	if(!pixmap_load_stream(istream, PIXMAP_FILEFORMAT_AUTO, &px, fmt)) {
		log_fatal("pixmap_load_stream() failed");
	}

	SDL_CloseIO(istream);

	pixmap_convert_inplace_realloc(&px, fmt);
	pixmap_flip_to_origin_inplace(&px, origin);

	Texture *tex = r_texture_create(&(TextureParams) {
		.class = TEXTURE_CLASS_2D,
		.type = r_texture_type_from_pixmap_format(fmt),
		.width = px.width,
		.height = px.height,
		.layers = 1,
		.filter.min = TEX_FILTER_LINEAR,
		.filter.mag = TEX_FILTER_LINEAR,
		.wrap.s = TEX_WRAP_REPEAT,
		.wrap.t = TEX_WRAP_REPEAT,
	});

	if(!tex) {
		log_fatal("r_texture_create() failed");
	}

	r_texture_fill(tex, 0, 0, &px);
	mem_free(px.data.untyped);

	return tex;
}

attr_unused
static Model test_renderer_load_iqm(const char *path) {
	IQMModel iqm;

	auto io = SDL_IOFromFile(path, "rb");

	if(!io) {
		log_sdl_error(LOG_FATAL, "SDL_IOFromFile");
		UNREACHABLE;
	}

	bool ok = iqm_load_stream(&iqm, path, io);
	SDL_CloseIO(io);

	if(!ok) {
		log_fatal("iqm_load_stream() failed");
	}

	Model mdl;
	r_model_add_static(&mdl, PRIM_TRIANGLES, iqm.num_vertices, iqm.vertices, iqm.num_indices, iqm.indices);

	mem_free(iqm.vertices);
	mem_free(iqm.indices);

	return mdl;
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
