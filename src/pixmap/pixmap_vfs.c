/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "pixmap_vfs.h"

#include "log.h"
#include "memory/scratch.h"
#include "vfs/public.h"

static bool pixmap_save_file_internal(const char *path, const Pixmap *src, const PixmapSaveOptions *opts) {
	SDL_IOStream *stream = vfs_open(path, VFS_MODE_WRITE);

	if(UNLIKELY(!stream)) {
		log_error("VFS error: %s", vfs_get_error());
		return false;
	}

	bool ok = pixmap_save_stream(stream, src, opts);
	SDL_CloseIO(stream);
	return ok;
}

bool pixmap_save_file(const char *path, const Pixmap *src, const PixmapSaveOptions *opts) {
	PixmapFileFormat fmt = PIXMAP_FILEFORMAT_AUTO;

	if(opts) {
		assert(opts->struct_size >= sizeof(PixmapSaveOptions));
		fmt = opts->file_format;
	}

	if(fmt == PIXMAP_FILEFORMAT_AUTO) {
		fmt = pixmap_fileformat_for_filename(path);
	}

	if(fmt == PIXMAP_FILEFORMAT_INVALID) {
		fmt = PIXMAP_FILEFORMAT_INTERNAL;
		log_warn("Couldn't determine file format from filename `%s`, assuming %s",
			path, NOT_NULL(pixmap_fileformat_name(fmt)));
	}

	if(!opts) {
		PixmapSaveOptions new_opts = PIXMAP_DEFAULT_SAVE_OPTIONS;
		new_opts.file_format = fmt;
		return pixmap_save_file_internal(path, src, &new_opts);
	}

	if(opts->file_format == fmt) {
		return pixmap_save_file_internal(path, src, opts);
	}

	return WITH_SCRATCH(s, ({
		PixmapSaveOptions *new_opts = marena_memdup(s, opts, opts->struct_size);
		new_opts->file_format = fmt;
		pixmap_save_file_internal(path, src, new_opts);
	}));
}

bool pixmap_load_file(const char *path, Pixmap *dst, PixmapFormat preferred_format) {
	SDL_IOStream *stream = vfs_open(path, VFS_MODE_READ);

	if(!stream) {
		log_error("VFS error: %s", vfs_get_error());
		return false;
	}

	bool result = pixmap_load_stream(stream, PIXMAP_FILEFORMAT_AUTO, dst, preferred_format);
	SDL_CloseIO(stream);
	return result;
}

static char *_pixmap_source_path(const char *prefix, const char *path, StringBuffer *sb) {
	strbuf_cat(sb, prefix);
	strbuf_cat(sb, path);
	char *base_path = strbuf_commit(sb);

	if(pixmap_check_filename(base_path) && vfs_query(base_path).exists) {
		return mem_strdup(base_path);
	}

	char *dot = strrchr(base_path, '.');

	if(dot) {
		*dot = 0;
	}

	for(PixmapFileFormat i = 0; i < PIXMAP_NUM_FILEFORMATS; ++i) {
		for(const char *const *l_ext = NOT_NULL(pixmap_fileformat_extensions(i)); *l_ext; ++l_ext) {
			strbuf_cat(sb, ".");
			strbuf_cat(sb, *l_ext);

			char *p = vfs_util_try_path("", base_path, sb->start);

			if(p != NULL) {
				return p;
			}

			strbuf_clear(sb);
		}
	}

	return NULL;
}

char *pixmap_source_path(const char *prefix, const char *path) {
	return WITH_SCRATCH(s, _pixmap_source_path(prefix, path, &STRBUF(s)));
}
