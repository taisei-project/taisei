/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "lang_glsl.h"
#include "shaderlib.h"

#include "log.h"
#include "memory/scratch.h"
#include "rwops/rwops_arena.h"
#include "util/io.h"
#include "util/stringops.h"
#include "vfs/pathutil.h"
#include "vfs/public.h"

enum {
	GLSL_INCLUDE_DEPTH = 42,
};

#define INCLUDE_DIRECTIVE "#include"
#define VERSION_DIRECTIVE "#version"

typedef struct GLSLParseState {
	const GLSLSourceOptions *options;
	ShaderSource *src;
	SDL_IOStream *dest;
	MemArena *scratch;
	RWArenaState stream_state;
	bool version_defined;
} GLSLParseState;

typedef struct GLSLFileParseState {
	GLSLParseState *global;
	int include_level;
	int lineno;
	const char *path;
	bool need_lineno_marker;
} GLSLFileParseState;

static inline void skip_space(char **p) {
	while(**p != 0 && isspace(**p)) {
		(*p)++;
	}
}

static void glsl_write_lineno(GLSLFileParseState *fstate) {
	if(!fstate->need_lineno_marker) {
		return;
	}

	SDL_RWprintf(fstate->global->dest, "// file: %s\n", fstate->path);
	SDL_RWprintf(fstate->global->dest, "#line %i %i\n", fstate->lineno, fstate->include_level);
	fstate->need_lineno_marker = false;
}

static void glsl_write_version(GLSLFileParseState *fstate, GLSLVersion version) {
	char buf[32];
	glsl_format_version(buf, sizeof(buf), fstate->global->src->lang.glsl.version);
	SDL_RWprintf(fstate->global->dest, "#version %s\n", buf);
	fstate->need_lineno_marker = true;
}

static void glsl_write_header(GLSLFileParseState *fstate) {
	SDL_RWprintf(
		fstate->global->dest,
		"#define %s_STAGE\n",
		fstate->global->options->stage == SHADER_STAGE_FRAGMENT ? "FRAG" : "VERT"
	);

	if(fstate->global->options->macros) {
		for(ShaderMacro *macro = fstate->global->options->macros; macro->name; ++macro) {
			SDL_RWprintf(fstate->global->dest, "#define %s %s\n", macro->name, STRORNULL(macro->value));
		}
	}

	fstate->need_lineno_marker = true;
}

static char* glsl_parse_include_directive(GLSLFileParseState *fstate, char *p) {
	char *filename;
	p += sizeof(INCLUDE_DIRECTIVE) - 1;
	skip_space(&p);

	if(*p != '"') {
		log_error("%s:%d: malformed %s directive: \" expected.", fstate->path, fstate->lineno, INCLUDE_DIRECTIVE);
		return NULL;
	}

	if(*p != 0) {
		p++;
	}

	filename = p;

	while(*p != 0 && *p != '"') {
		p++;
	}

	if(*p != '"') {
		log_error("%s:%d: malformed %s directive: second \" expected.", fstate->path, fstate->lineno, INCLUDE_DIRECTIVE);
		return NULL;
	}

	*p = 0;
	return filename;
}

static GLSLVersion glsl_parse_version_directive(GLSLFileParseState *fstate, char *p) {
	GLSLVersion version;
	p += sizeof(INCLUDE_DIRECTIVE) - 1;

	if(glsl_parse_version(p, &version) == p) {
		log_error("%s:%d: malformed %s directive.", fstate->path, fstate->lineno, VERSION_DIRECTIVE);
		return (GLSLVersion) { 0, GLSL_PROFILE_NONE };
	}

	return version;
}

static bool glsl_try_write_header(GLSLFileParseState *fstate) {
	if(fstate->global->version_defined) {
		return true;
	}

	if(fstate->global->options->version.version == 0) {
		log_error("%s:%d: no %s directive in shader and no default specified.", fstate->path, fstate->lineno, VERSION_DIRECTIVE);
		return false;
	}

	fstate->global->src->lang.glsl.version = fstate->global->options->version;
	fstate->global->version_defined = true;

	glsl_write_version(fstate, fstate->global->src->lang.glsl.version);
	glsl_write_header(fstate);

	return true;
}

static bool check_directive(char *p, const char *directive) {
	if(*p++ == '#') {
		skip_space(&p);
		return strstartswith(p, directive + 1);
	}

	return false;
}

static bool glsl_process_file(GLSLFileParseState *fstate) {
	if(fstate->include_level > GLSL_INCLUDE_DEPTH) {
		log_error("%s: max include depth reached. Maybe there is an include cycle?", fstate->path);
		return false;
	}

	SDL_IOStream *stream;
	GLSLSourceOpenCallback open = fstate->global->options->file_open_callback;

	if(open) {
		stream = open(fstate->path, fstate->global->options->file_open_callback_userdata);
	} else {
		stream = vfs_open(fstate->path, VFS_MODE_READ);
	}

	SDL_IOStream *dest = fstate->global->dest;

	if(!stream) {
		log_error("VFS error: %s", vfs_get_error());
		return false;
	}

	// TODO: Handle comments and line continuations properly.

	++fstate->lineno;
	glsl_write_lineno(fstate);

	auto scratch = fstate->global->scratch;
	auto scratch_snap = marena_snapshot(scratch);

	char *line;

	while(*(line = SDL_RWgets_arena(stream, scratch, NULL))) {
		char *p = line;
		skip_space(&p);

		if(check_directive(p, INCLUDE_DIRECTIVE)) {
			if(!glsl_try_write_header(fstate)) {
				SDL_CloseIO(stream);
				return false;
			}

			char *filename = glsl_parse_include_directive(fstate, p);

			if(filename == NULL) {
				SDL_CloseIO(stream);
				return false;
			}

			char include_abs_path[strlen(fstate->path) + strlen(filename) + 1];
			vfs_path_resolve_relative(include_abs_path, sizeof(include_abs_path), fstate->path, filename);

			SDL_RWprintf(dest, "// begin include (level %i)\n", fstate->include_level);

			bool include_ok = glsl_process_file(&(GLSLFileParseState) {
				.global = fstate->global,
				.include_level = fstate->include_level + 1,
				.path = include_abs_path,
				.need_lineno_marker = true,
			});

			SDL_RWprintf(dest, "// end include (level %i)\n", fstate->include_level);

			if(!include_ok) {
				SDL_CloseIO(stream);
				return false;
			}

			fstate->need_lineno_marker = true;
		} else if(check_directive(p, VERSION_DIRECTIVE)) {
			if(fstate->global->version_defined) {
				log_error("%s:%d: duplicate or misplaced %s directive", fstate->path, fstate->lineno, VERSION_DIRECTIVE);
				SDL_CloseIO(stream);
				return false;
			}

			GLSLVersion shader_v = glsl_parse_version_directive(fstate, p);

			if(shader_v.version == 0) {
				SDL_CloseIO(stream);
				return false;
			}

			GLSLVersion opt_v = fstate->global->options->version;
			bool version_matches = !memcmp(&shader_v, &opt_v, sizeof(GLSLVersion));

			if(opt_v.version == 0 || version_matches) {
				fstate->global->src->lang.glsl.version = shader_v;
			} else {
				char buf_opt[32], buf_shader[32];
				glsl_format_version(buf_opt, sizeof(buf_opt), opt_v);
				glsl_format_version(buf_shader, sizeof(buf_shader), shader_v);

				if(fstate->global->options->force_version) {
					log_warn(
						"%s:%d: version %s forced (source uses %s)",
						fstate->path, fstate->lineno, buf_opt, buf_shader
					);
					fstate->global->src->lang.glsl.version = opt_v;
				} else {
					log_debug(
						"%s:%d: source overrides version to %s (default is %s)",
						fstate->path, fstate->lineno, buf_shader, buf_opt
					);
					fstate->global->src->lang.glsl.version = shader_v;
				}
			}

			fstate->global->version_defined = true;

			glsl_write_version(fstate, fstate->global->src->lang.glsl.version);
			glsl_write_header(fstate);
		} else {
			if(*p && !glsl_try_write_header(fstate)) {
				SDL_CloseIO(stream);
				return false;
			}

			SDL_WriteIO(dest, line, strlen(line));
		}

		fstate->lineno++;

		glsl_write_lineno(fstate);
		marena_rollback(scratch, &scratch_snap);
	}

	SDL_CloseIO(stream);
	return true;
}

bool glsl_load_source(const char *path, ShaderSource *out, MemArena *arena, const GLSLSourceOptions *options) {
	*out = (typeof(*out)) {
		.lang.lang = SHLANG_GLSL,
		.lang.glsl.version = options->version,
		.stage = options->stage,
		.entrypoint = "main",
	};

	GLSLParseState pstate = {
		.src = out,
		.options = options,
		.scratch = acquire_scratch_arena(),
	};

	pstate.dest = NOT_NULL(SDL_RWArena(arena, 1024, &pstate.stream_state));

	GLSLFileParseState fstate = {
		.global = &pstate,
		.path = path,
	};

	bool result = glsl_process_file(&fstate);
	release_scratch_arena(pstate.scratch);

	if(result) {
		SDL_WriteU8(pstate.dest, 0);
		out->content = pstate.stream_state.buffer;
		out->content_size = pstate.stream_state.io_offset;
		// log_debug("*****\n%s\n*****", out->content);
		assert(strlen(out->content) == out->content_size - 1);
	}

	SDL_CloseIO(pstate.dest);
	return result;
}

static inline const char *profile_suffix(GLSLVersion version) {
	if(version.version == 100) {
		return "";
	}

	switch(version.profile) {
		case GLSL_PROFILE_CORE:          return " core";
		case GLSL_PROFILE_COMPATIBILITY: return " compatibility";
		case GLSL_PROFILE_ES:            return " es";
		case GLSL_PROFILE_NONE:          return "";
		default: UNREACHABLE;
	}
}

int glsl_format_version(char *buf, size_t bufsize, GLSLVersion version) {
	return snprintf(buf, bufsize, "%i%s", version.version, profile_suffix(version));
}

char *glsl_parse_version(const char *str, GLSLVersion *out_version) {
	char *end, *p = (char*)str, *orig = (char*)str;

	skip_space(&p);
	out_version->version = strtol(p, &end, 10);
	out_version->profile = GLSL_PROFILE_NONE;

	if(p == end) {
		log_error("Malformed version '%s': version number expected.", str);
		return orig;
	}

	if(out_version->version < 100) {
		log_error("Malformed version '%s': bad version number.", str);
		return orig;
	}

	p = end;
	skip_space(&p);

	if(out_version->version == 100) {
		out_version->profile = GLSL_PROFILE_ES;
	}

	if(*p == 0) {
		return p;
	}

	if(out_version->version != 100) {
		if(!strncmp(p, "core", 4)) {
			out_version->profile = GLSL_PROFILE_CORE;
			p += 4;
		} else if(!strncmp(p, "compatibility", 13)) {
			out_version->profile = GLSL_PROFILE_COMPATIBILITY;
			p += 13;
		} else if(!strncmp(p, "es", 2)) {
			out_version->profile = GLSL_PROFILE_ES;
			p += 2;
		} else {
			log_warn("Unknown profile in version '%s' ignored", str);
		}
	}

	return p;
}

bool glsl_version_supports_instanced_rendering(GLSLVersion v) {
	if(v.profile == GLSL_PROFILE_ES) {
		return v.version >= 300;
	} else {
		return v.version >= 330;
	}
}
