/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "shader_glsl.h"
#include "rwops/rwops_autobuf.h"
#include "vfs/pathutil.h"
#include "vfs/public.h"
#include "util.h"

enum {
	GLSL_INCLUDE_DEPTH = 42,
};

#define INCLUDE_DIRECTIVE "#include"
#define VERSION_DIRECTIVE "#version"

typedef struct GLSLParseState {
	const GLSLSourceOptions *options;
	ShaderSource *src;
	SDL_RWops *dest;
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

static const char* profile_suffix(GLSLProfile prof) {
	switch(prof) {
		case GLSL_PROFILE_CORE:          return " core";
		case GLSL_PROFILE_COMPATIBILITY: return " compatibility";
		case GLSL_PROFILE_ES:            return " es";
		case GLSL_PROFILE_NONE:          return "";
		default: UNREACHABLE;
	}
}

static void glsl_write_version(GLSLFileParseState *fstate, GLSLVersion version) {
	const char *profile = profile_suffix(fstate->global->src->lang.glsl.version.profile);
	SDL_RWprintf(fstate->global->dest, "#version %i%s\n", fstate->global->src->lang.glsl.version.version, profile);
	fstate->need_lineno_marker = true;
}

static void glsl_write_header(GLSLFileParseState *fstate) {
	SDL_RWprintf(
		fstate->global->dest,
		"#define %s_STAGE\n",
		fstate->global->options->stage == SHADER_STAGE_FRAGMENT ? "FRAG" : "VERT"
	);

	if(fstate->global->options->macros) {
		for(GLSLMacro *macro = fstate->global->options->macros; macro->name; ++macro) {
			SDL_RWprintf(fstate->global->dest, "#define %s %s\n", macro->name, macro->value);
		}
	}

	fstate->need_lineno_marker = true;
}

static char* glsl_parse_include(GLSLFileParseState *fstate, char *p) {
	char *filename;
	p += sizeof(INCLUDE_DIRECTIVE) - 1;
	skip_space(&p);

	if(*p != '"') {
		log_warn("%s:%d: malformed %s directive: \" expected.", fstate->path, fstate->lineno, INCLUDE_DIRECTIVE);
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
		log_warn("%s:%d: malformed %s directive: second \" expected.", fstate->path, fstate->lineno, INCLUDE_DIRECTIVE);
		return NULL;
	}

	*p = 0;
	return filename;
}

static GLSLVersion glsl_parse_version(GLSLFileParseState *fstate, char *p) {
	GLSLVersion version = { 0, GLSL_PROFILE_NONE };
	char *end;
	p += sizeof(INCLUDE_DIRECTIVE) - 1;
	skip_space(&p);

	version.version = strtol(p, &end, 10);

	if(p == end) {
		log_warn("%s:%d: malformed %s directive: version number expected.", fstate->path, fstate->lineno, VERSION_DIRECTIVE);
		return version;
	}

	if(version.version < 100) {
		log_warn("%s:%d: malformed %s directive: bad version number %i.", fstate->path, fstate->lineno, VERSION_DIRECTIVE, version.version);
		version.version = 0;
		return version;
	}

	p = end;
	skip_space(&p);

	if(*p == 0) {
		return version;
	}

	if(!strncmp(p, "core", 4)) {
		version.profile = GLSL_PROFILE_CORE;
	} else if(!strncmp(p, "compatibility", 13)) {
		version.profile = GLSL_PROFILE_COMPATIBILITY;
	} else if(!strncmp(p, "es", 2)) {
		version.profile = GLSL_PROFILE_ES;
	} else {
		log_warn("%s:%d: unknown profile in %s directive.", fstate->path, fstate->lineno, VERSION_DIRECTIVE);
	}

	return version;
}

bool glsl_try_write_header(GLSLFileParseState *fstate) {
	if(fstate->global->version_defined) {
		return true;
	}

	if(fstate->global->options->version.version == 0) {
		log_warn("%s:%d: no %s directive in shader and no default specified.", fstate->path, fstate->lineno, VERSION_DIRECTIVE);
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
		log_warn("%s: max include depth reached. Maybe there is an include cycle?", fstate->path);
		return false;
	}

	SDL_RWops *stream = vfs_open(fstate->path, VFS_MODE_READ);
	SDL_RWops *dest = fstate->global->dest;

	if(!stream) {
		log_warn("VFS error: %s", vfs_get_error());
		return false;
	}

	// TODO: Remove this dumb limitation; also handle comments and line continuations properly.
	char linebuf[1024];

	++fstate->lineno;
	glsl_write_lineno(fstate);

	while(SDL_RWgets(stream, linebuf, sizeof(linebuf))) {
		char *p = linebuf;
		skip_space(&p);

		if(check_directive(p, INCLUDE_DIRECTIVE)) {
			if(!glsl_try_write_header(fstate)) {
				SDL_RWclose(stream);
				return false;
			}

			char *filename = glsl_parse_include(fstate, p);

			if(filename == NULL) {
				SDL_RWclose(stream);
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
				SDL_RWclose(stream);
				return false;
			}

			fstate->need_lineno_marker = true;
		} else if(check_directive(p, VERSION_DIRECTIVE)) {
			if(fstate->global->version_defined) {
				log_warn("%s:%d: duplicate or misplaced %s directive", fstate->path, fstate->lineno, VERSION_DIRECTIVE);
				SDL_RWclose(stream);
				return false;
			}

			GLSLVersion shader_v = glsl_parse_version(fstate, p);

			if(shader_v.version == 0) {
				SDL_RWclose(stream);
				return false;
			}

			GLSLVersion opt_v = fstate->global->options->version;
			bool version_matches = !memcmp(&shader_v, &opt_v, sizeof(GLSLVersion));

			if(opt_v.version == 0 || version_matches) {
				fstate->global->src->lang.glsl.version = shader_v;
			} else if(fstate->global->options->force_version) {
				log_warn(
					"%s:%d: version %i%s forced (source uses %i%s)",
					fstate->path, fstate->lineno,
					opt_v.version, profile_suffix(opt_v.profile),
					shader_v.version, profile_suffix(shader_v.profile)
				);
				fstate->global->src->lang.glsl.version = opt_v;
			} else {
				log_warn(
					"%s:%d: source overrides version to %i%s (default is %i%s)",
					fstate->path, fstate->lineno,
					shader_v.version, profile_suffix(shader_v.profile),
					opt_v.version, profile_suffix(opt_v.profile)
				);
				fstate->global->src->lang.glsl.version = shader_v;
			}

			fstate->global->version_defined = true;

			glsl_write_version(fstate, fstate->global->src->lang.glsl.version);
			glsl_write_header(fstate);
		} else {
			if(*p && !glsl_try_write_header(fstate)) {
				SDL_RWclose(stream);
				return false;
			}

			SDL_RWwrite(dest, linebuf, 1, strlen(linebuf));
		}

		fstate->lineno++;
		glsl_write_lineno(fstate);
	}

	SDL_RWclose(stream);
	return true;
}

bool glsl_load_source(const char *path, ShaderSource *out, const GLSLSourceOptions *options) {
	void *bufdata_ptr;
	SDL_RWops *out_buf = SDL_RWAutoBuffer(&bufdata_ptr, 1024);
	assert(out_buf != NULL);

	out->lang.lang = SHLANG_GLSL;
	out->lang.glsl.version = options->version;
	out->stage = options->stage;
	out->content = NULL;
	out->content_size = 0;

	GLSLParseState pstate = { 0 };
	pstate.dest = out_buf;
	pstate.src = out;
	pstate.options = options;

	GLSLFileParseState fstate = { 0 };
	fstate.global = &pstate;
	fstate.path = path;

	bool result = glsl_process_file(&fstate);

	if(result) {
		SDL_WriteU8(out_buf, 0);
		out->content_size = strlen(bufdata_ptr) + 1;
		out->content = malloc(out->content_size);
		memcpy(out->content, bufdata_ptr, out->content_size);
	}

	SDL_RWclose(out_buf);
	return result;
}
