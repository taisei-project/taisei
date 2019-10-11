/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "audio.h"
#include "backend.h"
#include "resource/resource.h"
#include "global.h"

#define B (_a_backend.funcs)

CurrentBGM current_bgm = { .name = NULL };

static char *saved_bgm;
static ht_str2int_t sfx_volumes;

static struct enqueued_sound {
	LIST_INTERFACE(struct enqueued_sound);
	char *name;
	int time;
	int cooldown;
	bool replace;
} *sound_queue;

static void play_sound_internal(const char *name, bool is_ui, int cooldown, bool replace, int delay) {
	if(!audio_output_works() || global.frameskip) {
		return;
	}

	if(delay > 0) {
		struct enqueued_sound *s = malloc(sizeof(struct enqueued_sound));
		s->time = global.frames + delay;
		s->name = strdup(name);
		s->cooldown = cooldown;
		s->replace = replace;
		list_push(&sound_queue, s);
		return;
	}

	if(taisei_is_skip_mode_enabled()) {
		return;
	}

	Sound *snd = get_sound(name);

	if(!snd || (!is_ui && snd->lastplayframe + 3 + cooldown >= global.frames)) {
		return;
	}

	snd->lastplayframe = global.frames;

	(replace ? B.sound_play_or_restart : B.sound_play)
		(snd->impl, is_ui ? SNDGROUP_UI : SNDGROUP_MAIN);
}

static void* discard_enqueued_sound(List **queue, List *vsnd, void *arg) {
	struct enqueued_sound *snd = (struct enqueued_sound*)vsnd;
	free(snd->name);
	free(list_unlink(queue, vsnd));
	return NULL;
}

static void* play_enqueued_sound(struct enqueued_sound **queue, struct enqueued_sound *snd, void *arg) {
	if(!taisei_is_skip_mode_enabled()) {
		play_sound_internal(snd->name, false, snd->cooldown, snd->replace, 0);
	}

	free(snd->name);
	free(list_unlink(queue, snd));
	return NULL;
}

void play_sound(const char *name) {
	play_sound_internal(name, false, 0, false, 0);
}

void play_sound_ex(const char *name, int cooldown, bool replace) {
	play_sound_internal(name, false, cooldown, replace, 0);
}

void play_sound_delayed(const char *name, int cooldown, bool replace, int delay) {
	play_sound_internal(name, false, cooldown, replace, delay);
}

void play_ui_sound(const char *name) {
	play_sound_internal(name, true, 0, true, 0);
}

void play_loop(const char *name) {
	if(!audio_output_works() || global.frameskip) {
		return;
	}

	Sound *snd = get_sound(name);

	if(!snd) {
		return;
	}

	if(!snd->islooping) {
		B.sound_loop(snd->impl, SNDGROUP_MAIN);
		snd->islooping = true;
	}

	if(snd->islooping == LS_LOOPING) {
		snd->lastplayframe = global.frames;
	}
}

static void* reset_sounds_callback(const char *name, Resource *res, void *arg) {
	bool reset = (intptr_t)arg;
	Sound *snd = res->data;

	if(snd) {
		if(reset) {
			snd->lastplayframe = 0;
		}

		if(snd->islooping && (global.frames > snd->lastplayframe + LOOPTIMEOUTFRAMES || reset)) {
			B.sound_stop_loop(snd->impl);
			snd->islooping = LS_FADEOUT;
		}

		if(snd->islooping && (global.frames > snd->lastplayframe + LOOPTIMEOUTFRAMES + LOOPFADEOUT*60/1000. || reset)) {
			snd->islooping = LS_OFF;
		}
	}

	return NULL;
}

void reset_sounds(void) {
	resource_for_each(RES_SFX, reset_sounds_callback, (void*)true);
	list_foreach(&sound_queue, discard_enqueued_sound, NULL);
}

void update_sounds(void) {
	resource_for_each(RES_SFX, reset_sounds_callback, (void*)false);

	for(struct enqueued_sound *s = sound_queue, *next; s; s = next) {
		next = (struct enqueued_sound*)s->next;

		if(s->time <= global.frames) {
			play_enqueued_sound(&sound_queue, s, NULL);
		}
	}
}

void pause_sounds(void) {
	B.sound_pause_all(SNDGROUP_MAIN);
}

void resume_sounds(void) {
	B.sound_resume_all(SNDGROUP_MAIN);
}

void stop_sounds(void) {
	B.sound_stop_all(SNDGROUP_MAIN);
}

Sound* get_sound(const char *name) {
	return get_resource_data(RES_SFX, name, RESF_OPTIONAL);
}

Music* get_music(const char *name) {
	return get_resource_data(RES_BGM, name, RESF_OPTIONAL);
}

static bool store_sfx_volume(const char *key, const char *val, void *data) {
	int vol = atoi(val);

	if(vol < 0 || vol > 128) {
		log_warn("Volume %i for sfx %s is out of range; must be within [0, 128]", vol, key);
		vol = vol < 0 ? 0 : 128;
	}

	log_debug("Default volume for %s is now %i", key, vol);

	if(vol != DEFAULT_SFX_VOLUME) {
		ht_set(&sfx_volumes, key, vol);
	}

	return true;
}

static void load_config_files(void) {
	ht_create(&sfx_volumes);
	parse_keyvalue_file_cb(SFX_PATH_PREFIX "volumes.conf", store_sfx_volume, NULL);
}

static inline char* get_bgm_title(char *name) {
	MusicMetadata *meta = get_resource_data(RES_BGM_METADATA, name, RESF_OPTIONAL);
	return meta ? meta->title : NULL;
}

int get_default_sfx_volume(const char *sfx) {
	return ht_get(&sfx_volumes, sfx, DEFAULT_SFX_VOLUME);
}

void resume_bgm(void) {
	start_bgm(current_bgm.name); // In most cases it just unpauses existing music.
}

static void stop_bgm_internal(bool pause, double fadetime) {
	if(!current_bgm.name) {
		return;
	}

	current_bgm.started_at = -1;

	if(!pause) {
		stralloc(&current_bgm.name, NULL);
	}

	if(B.music_is_playing() && !B.music_is_paused()) {
		if(pause) {
			B.music_pause();
			log_debug("BGM paused");
		} else if(fadetime > 0) {
			B.music_fade(fadetime);
			log_debug("BGM fading out");
		} else {
			B.music_stop();
			log_debug("BGM stopped");
		}
	} else {
		log_debug("No BGM was playing");
	}
}

void stop_bgm(bool force) {
	stop_bgm_internal(!force, false);
}

void fade_bgm(double fadetime) {
	stop_bgm_internal(false, fadetime);
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
		B.music_stop();

		stralloc(&current_bgm.name, name);

		if((current_bgm.music = get_music(name)) == NULL) {
			log_warn("BGM '%s' does not exist", current_bgm.name);
			stop_bgm(true);
			free(current_bgm.name);
			current_bgm.name = NULL;
			return;
		}
	}

	if(B.music_is_paused()) {
		B.music_resume();
	}

	if(B.music_is_playing()) {
		return;
	}

	if(!B.music_play(current_bgm.music->impl)) {
		return;
	}

	// Support drawing BGM title in game loop (only when music changed!)
	if((current_bgm.title = get_bgm_title(current_bgm.name)) != NULL) {
		current_bgm.started_at = global.frames;
		// Boss BGM title color may differ from the one at beginning of stage
		current_bgm.isboss = strendswith(current_bgm.name, "boss");
	} else {
		current_bgm.started_at = -1;
	}

	log_info("Started %s", (current_bgm.title ? current_bgm.title : current_bgm.name));
}

static bool audio_config_updated(SDL_Event *evt, void *arg) {
	if(config_get_int(CONFIG_MUTE_AUDIO)) {
		B.sound_set_global_volume(0.0);
		B.music_set_global_volume(0.0);
	} else {
		B.sound_set_global_volume(config_get_float(CONFIG_SFX_VOLUME));
		B.music_set_global_volume(config_get_float(CONFIG_BGM_VOLUME));
	}

	return false;
}

void audio_init(void) {
	load_config_files();
	audio_backend_init();
	events_register_handler(&(EventHandler) {
		audio_config_updated, NULL, EPRIO_SYSTEM, MAKE_TAISEI_EVENT(TE_CONFIG_UPDATED)
	});
	audio_config_updated(NULL, NULL);
}

void audio_shutdown(void) {
	events_unregister_handler(audio_config_updated);
	B.shutdown();
	ht_destroy(&sfx_volumes);
}

bool audio_output_works(void) {
	return B.output_works();
}

void audio_music_set_position(double pos) {
	B.music_set_position(pos);
}
