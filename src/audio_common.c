/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include <string.h>
#include <stdio.h>

#include "audio.h"
#include "resource/resource.h"
#include "global.h"

static char *saved_bgm;
static Hashtable *bgm_descriptions;
static Hashtable *sfx_volumes;
CurrentBGM current_bgm = { .name = NULL };

static void play_sound_internal(const char *name, bool is_ui, int cooldown, bool replace) {
	if(!audio_backend_initialized() || global.frameskip) {
		return;
	}

	Sound *snd = get_sound(name);

	if(!snd || (!is_ui && snd->lastplayframe + 3 + cooldown >= global.frames) || snd->islooping) {
		return;
	}

	snd->lastplayframe = global.frames;

	(replace ? audio_backend_sound_play_or_restart : audio_backend_sound_play)
		(snd->impl, is_ui ? SNDGROUP_UI : SNDGROUP_MAIN);
}

void play_sound(const char *name) {
	play_sound_internal(name, false, 0, false);
}

void play_sound_ex(const char *name, int cooldown, bool replace) {
	play_sound_internal(name, false, cooldown, replace);
}

void play_ui_sound(const char *name) {
	play_sound_internal(name, true, 0, true);
}

void play_loop(const char *name) {
	if(!audio_backend_initialized() || global.frameskip) {
		return;
	}

	Sound *snd = get_sound(name);

	if(!snd) {
		return;
	}

	snd->lastplayframe = global.frames;
	if(!snd->islooping) {
		audio_backend_sound_loop(snd->impl, SNDGROUP_MAIN);
		snd->islooping = true;
	}
}

void reset_sounds(void) {
	Resource *snd;
	for(HashtableIterator *i = hashtable_iter(resources.handlers[RES_SFX].mapping);
			hashtable_iter_next(i, 0, (void**)&snd);) {
		snd->sound->lastplayframe = 0;
		if(snd->sound->islooping) {
			snd->sound->islooping = false;
			audio_backend_sound_stop_loop(snd->sound->impl);
		}
	}
}

void update_sounds(void) {
	Resource *snd;
	for(HashtableIterator *i = hashtable_iter(resources.handlers[RES_SFX].mapping);
			hashtable_iter_next(i, 0, (void**)&snd);) {
		if(snd->sound->islooping && global.frames > snd->sound->lastplayframe + LOOPTIMEOUTFRAMES) {
			snd->sound->islooping = false;
			audio_backend_sound_stop_loop(snd->sound->impl);
		}
	}
}

void pause_sounds(void) {
	audio_backend_sound_pause_all(SNDGROUP_MAIN);
}

void resume_sounds(void) {
	audio_backend_sound_resume_all(SNDGROUP_MAIN);
}

void stop_sounds(void) {
	audio_backend_sound_stop_all(SNDGROUP_MAIN);
}

Sound* get_sound(const char *name) {
	Resource *res = get_resource(RES_SFX, name, RESF_OPTIONAL);
	return res ? res->sound : NULL;
}

Music* get_music(const char *name) {
	Resource *res = get_resource(RES_BGM, name, RESF_OPTIONAL);
	return res ? res->music : NULL;
}

static void sfx_cfg_volume_callback(ConfigIndex idx, ConfigValue v) {
	audio_backend_set_sfx_volume(config_set_float(idx, v.f));
}

static void bgm_cfg_volume_callback(ConfigIndex idx, ConfigValue v) {
	audio_backend_set_bgm_volume(config_set_float(idx, v.f));
}

static void store_sfx_volume(const char *key, const char *val, Hashtable *ht) {
	int vol = atoi(val);

	if(vol < 0 || vol > 128) {
		log_warn("Volume %i for sfx %s is out of range; must be within [0, 128]", vol, key);
		vol = vol < 0 ? 0 : 128;
	}

	log_debug("Default volume for %s is now %i", key, vol);

	if(vol != DEFAULT_SFX_VOLUME) {
		hashtable_set_string(ht, key, (void*)(intptr_t)vol);
	}
}

static void load_config_files(void) {
	bgm_descriptions = parse_keyvalue_file(BGM_PATH_PREFIX "bgm.conf", HT_DYNAMIC_SIZE);
	sfx_volumes = hashtable_new_stringkeys(HT_DYNAMIC_SIZE);
	parse_keyvalue_file_cb(SFX_PATH_PREFIX "volumes.conf", (KVCallback)store_sfx_volume, sfx_volumes);
}

static inline char* get_bgm_desc(char *name) {
	Music *music = get_music(name);
	assert(music != NULL);

	if(music->title) {
		return music->title;
	}

	return bgm_descriptions ? (char*)hashtable_get_string(bgm_descriptions, name) : NULL;
}

int get_default_sfx_volume(const char *sfx) {
	void *v = hashtable_get_string(sfx_volumes, sfx);

	if(v != NULL) {
		return (intptr_t)v;
	}

	return DEFAULT_SFX_VOLUME;
}

void resume_bgm(void) {
	start_bgm(current_bgm.name); // In most cases it just unpauses existing music.
}

static void stop_bgm_internal(bool pause, bool fade) {
	if(!current_bgm.name) {
		return;
	}

	current_bgm.started_at = -1;

	if(!pause) {
		stralloc(&current_bgm.name, NULL);
	}

	if(audio_backend_music_is_playing() && !audio_backend_music_is_paused()) {
		if(pause) {
			audio_backend_music_pause();
			log_debug("BGM paused");
		} else if(fade) {
			audio_backend_music_fade((FPS * FADE_TIME) / 2000.0);
			log_debug("BGM fading out");
		} else {
			audio_backend_music_stop();
			log_debug("BGM stopped");
		}
	} else {
		log_debug("No BGM was playing");
	}
}

void stop_bgm(bool force) {
	stop_bgm_internal(!force, false);
}

void fade_bgm(void) {
	stop_bgm_internal(false, true);
}

void save_bgm(void) {
	// XXX: this is broken
	stralloc(&saved_bgm, current_bgm.name);
}

void restore_bgm(void) {
	// XXX: this is broken
	start_bgm(saved_bgm);
	free(saved_bgm);
	saved_bgm = NULL;
}

void start_bgm(const char *name) {
	if(!name || !*name) {
		stop_bgm(false);
		return;
	}

	// if BGM has changed, change it and start from beginning
	if(!current_bgm.name || strcmp(name, current_bgm.name)) {
		audio_backend_music_stop();

		stralloc(&current_bgm.name, name);

		if((current_bgm.music = get_music(name)) == NULL) {
			log_warn("BGM '%s' does not exist", current_bgm.name);
			stop_bgm(true);
			free(current_bgm.name);
			current_bgm.name = NULL;
			return;
		}
	}

	if(audio_backend_music_is_paused()) {
		audio_backend_music_resume();
	}

	if(audio_backend_music_is_playing()) {
		return;
	}

	if(!audio_backend_music_play(current_bgm.music->impl)) {
		return;
	}

	// Support drawing BGM title in game loop (only when music changed!)
	if((current_bgm.title = get_bgm_desc(current_bgm.name)) != NULL) {
		current_bgm.started_at = global.frames;
		// Boss BGM title color may differ from the one at beginning of stage
		current_bgm.isboss = strendswith(current_bgm.name, "boss");
	} else {
		current_bgm.started_at = -1;
	}

	log_info("Started %s", (current_bgm.title ? current_bgm.title : current_bgm.name));
}

void audio_init(void) {
	load_config_files();
	audio_backend_init();
	config_set_callback(CONFIG_SFX_VOLUME, sfx_cfg_volume_callback);
	config_set_callback(CONFIG_BGM_VOLUME, bgm_cfg_volume_callback);
}

void audio_shutdown(void) {
	audio_backend_shutdown();

	if(bgm_descriptions) {
		hashtable_foreach(bgm_descriptions, hashtable_iter_free_data, NULL);
		hashtable_free(bgm_descriptions);
		bgm_descriptions = NULL;
	}

	if(sfx_volumes) {
		hashtable_free(sfx_volumes);
		sfx_volumes = NULL;
	}
}
