/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "replay.h"
#include "struct.h"
#include "stage.h"

#include "rwops/rwops_stdiofp.h"

void replay_destroy_events(Replay *rpy) {
	dynarray_foreach_elem(&rpy->stages, ReplayStage *stg, {
		replay_stage_destroy_events(stg);
	});
}

void replay_reset(Replay *rpy) {
	replay_destroy_events(rpy);
	dynarray_free_data(&rpy->stages);
	mem_free(rpy->playername);
	memset(rpy, 0, sizeof(Replay));
}

static char *replay_getpath(const char *name, bool ext) {
	return ext ?
		strfmt("storage/replays/%s.%s", name, REPLAY_EXTENSION) :
		strfmt("storage/replays/%s",    name);
}

bool replay_save(Replay *rpy, const char *name) {
	char *p = replay_getpath(name, !strendswith(name, REPLAY_EXTENSION));
	char *sp = vfs_repr(p, true);
	log_info("Saving %s", sp);
	mem_free(sp);

	SDL_IOStream *file = vfs_open(p, VFS_MODE_WRITE);
	mem_free(p);

	if(!file) {
		log_error("VFS error: %s", vfs_get_error());
		return false;
	}

	bool result = replay_write(rpy, file, REPLAY_STRUCT_VERSION_WRITE);
	SDL_CloseIO(file);
	vfs_sync(VFS_SYNC_STORE, NO_CALLCHAIN);
	return result;
}

static const char *replay_mode_string(ReplayReadMode mode) {
	if((mode & REPLAY_READ_ALL) == REPLAY_READ_ALL) {
		return "full";
	}

	if(mode & REPLAY_READ_META) {
		return "meta";
	}

	if(mode & REPLAY_READ_EVENTS) {
		return "events";
	}

	log_fatal("Bad mode %i", mode);
}

bool replay_load_vfspath(Replay *rpy, const char *path, ReplayReadMode mode) {
	char *sp = vfs_repr(path, true);
	log_info("Loading %s (%s)", sp, replay_mode_string(mode));

	SDL_IOStream *file = vfs_open(path, VFS_MODE_READ);

	if(!file) {
		log_error("VFS error: %s", vfs_get_error());
		mem_free(sp);
		return false;
	}

	bool result = replay_read(rpy, file, mode, sp);

	mem_free(sp);
	SDL_CloseIO(file);
	return result;
}

bool replay_load(Replay *rpy, const char *name, ReplayReadMode mode) {
	char *p = replay_getpath(name, !strendswith(name, REPLAY_EXTENSION));
	bool result = replay_load_vfspath(rpy, p, mode);
	mem_free(p);
	return result;
}

bool replay_load_syspath(Replay *rpy, const char *path, ReplayReadMode mode) {
	log_info("Loading %s (%s)", path, replay_mode_string(mode));
	SDL_IOStream *file;

	if(!strcmp(path, "-")) {
		file = SDL_RWFromFP(stdin, false);
		if(!file) {
			log_sdl_error(LOG_ERROR, "SDL_RWFromFP");
			return false;
		}
	} else {
		file = SDL_IOFromFile(path, "rb");
		if(!file) {
			log_sdl_error(LOG_ERROR, "SDL_IOFromFile");
			return false;
		}
	}


	bool result = replay_read(rpy, file, mode, path);

	SDL_CloseIO(file);
	return result;
}

bool replay_save_syspath(Replay *rpy, const char *path, uint16_t version) {
	log_info("Saving %s", path);
	SDL_IOStream *file;

	if(!strcmp(path, "-")) {
		file = SDL_RWFromFP(stdout, false);
		if(!file) {
			log_sdl_error(LOG_ERROR, "SDL_RWFromFP");
			return false;
		}
	} else {
		file = SDL_IOFromFile(path, "wb");
		if(!file) {
			log_sdl_error(LOG_ERROR, "SDL_IOFromFile");
			return false;
		}
	}

	bool result = replay_write(rpy, file, version);
	SDL_CloseIO(file);
	return result;
}

int replay_find_stage_idx(Replay *rpy, uint8_t stageid) {
	dynarray_foreach(&rpy->stages, int i, ReplayStage *stg, {
		if(stg->stage == stageid) {
			return i;
		}
	});

	log_warn("Stage %X was not found in the replay", stageid);
	return -1;
}
