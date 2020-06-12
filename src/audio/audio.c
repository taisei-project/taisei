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
#include "resource/bgm.h"
#include "resource/sfx.h"
#include "global.h"

#define B (_a_backend.funcs)

static ht_str2int_t sfx_volumes;

static struct enqueued_sound {
	LIST_INTERFACE(struct enqueued_sound);
	char *name;
	int time;
	int cooldown;
	bool replace;
} *sound_queue;

static SFXPlayID play_sound_internal(const char *name, bool is_ui, int cooldown, bool replace, int delay) {
	if(!audio_output_works() || global.frameskip) {
		return 0;
	}

	if(delay > 0) {
		struct enqueued_sound *s = malloc(sizeof(struct enqueued_sound));
		s->time = global.frames + delay;
		s->name = strdup(name);
		s->cooldown = cooldown;
		s->replace = replace;
		list_push(&sound_queue, s);
		return 0;
	}

	if(taisei_is_skip_mode_enabled()) {
		return 0;
	}

	SFX *sfx = res_sfx(name);

	if(!sfx || (!is_ui && sfx->lastplayframe + 3 + cooldown >= global.frames)) {
		return 0;
	}

	sfx->lastplayframe = global.frames;

	AudioBackendSFXGroup group = is_ui ? SFXGROUP_UI : SFXGROUP_MAIN;
	SFXPlayID sid;

	if(replace) {
		sid = B.sfx_play_or_restart(sfx->impl, group);
	} else {
		sid = B.sfx_play(sfx->impl, group);
	}

	return sid;
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

SFXPlayID play_sfx(const char *name) {
	return play_sound_internal(name, false, 0, false, 0);
}

SFXPlayID play_sfx_ex(const char *name, int cooldown, bool replace) {
	return play_sound_internal(name, false, cooldown, replace, 0);
}

void play_sfx_delayed(const char *name, int cooldown, bool replace, int delay) {
	play_sound_internal(name, false, cooldown, replace, delay);
}

void play_sfx_ui(const char *name) {
	play_sound_internal(name, true, 0, true, 0);
}

void stop_sound(SFXPlayID sid) {
	if(sid) {
		B.sfx_stop_id(sid);
	}
}

void replace_sfx(SFXPlayID sid, const char *name) {
	stop_sound(sid);
	play_sfx(name);
}

void play_sfx_loop(const char *name) {
	if(!audio_output_works() || global.frameskip) {
		return;
	}

	SFX *sfx = res_sfx(name);

	if(!sfx) {
		return;
	}

	if(!sfx->islooping) {
		B.sfx_loop(sfx->impl, SFXGROUP_MAIN);
		sfx->islooping = true;
	}

	if(sfx->islooping == LS_LOOPING) {
		sfx->lastplayframe = global.frames;
	}
}

static void* reset_sounds_callback(const char *name, Resource *res, void *arg) {
	bool reset = (intptr_t)arg;
	SFX *sfx = res->data;

	if(sfx) {
		if(reset) {
			sfx->lastplayframe = 0;
		}

		if(sfx->islooping && (global.frames > sfx->lastplayframe + LOOPTIMEOUTFRAMES || reset)) {
			B.sfx_stop_loop(sfx->impl);
			sfx->islooping = LS_FADEOUT;
		}

		if(sfx->islooping && (global.frames > sfx->lastplayframe + LOOPTIMEOUTFRAMES + LOOPFADEOUT*60/1000. || reset)) {
			sfx->islooping = LS_OFF;
		}
	}

	return NULL;
}

void reset_all_sfx(void) {
	resource_for_each(RES_SFX, reset_sounds_callback, (void*)true);
	list_foreach(&sound_queue, discard_enqueued_sound, NULL);
}

void update_all_sfx(void) {
	resource_for_each(RES_SFX, reset_sounds_callback, (void*)false);

	for(struct enqueued_sound *s = sound_queue, *next; s; s = next) {
		next = (struct enqueued_sound*)s->next;

		if(s->time <= global.frames) {
			play_enqueued_sound(&sound_queue, s, NULL);
		}
	}
}

void pause_all_sfx(void) {
	B.sfx_pause_all(SFXGROUP_MAIN);
}

void resume_all_sfx(void) {
	B.sfx_resume_all(SFXGROUP_MAIN);
}

void stop_all_sfx(void) {
	B.sfx_stop_all(SFXGROUP_MAIN);
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

int get_default_sfx_volume(const char *sfx) {
	return ht_get(&sfx_volumes, sfx, DEFAULT_SFX_VOLUME);
}

static bool audio_config_updated(SDL_Event *evt, void *arg) {
	if(config_get_int(CONFIG_MUTE_AUDIO)) {
		B.sfx_set_global_volume(0.0);
		B.bgm_set_global_volume(0.0);
	} else {
		B.sfx_set_global_volume(config_get_float(CONFIG_SFX_VOLUME));
		B.bgm_set_global_volume(config_get_float(CONFIG_BGM_VOLUME));
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

double audioutil_loopaware_position(double rt_pos, double duration, double loop_start) {
	loop_start = clamp(loop_start, 0, duration);

	if(rt_pos < loop_start) {
		return fmax(0, rt_pos);
	}

	return fmod(rt_pos - loop_start, duration - loop_start) + loop_start;
}

bool audio_bgm_play(BGM *bgm, bool loop, double position, double fadein) {
	BGM *current = B.bgm_current();

	if(current == bgm) {
		return false;
	}

	return audio_bgm_play_or_restart(bgm, loop, position, fadein);
}

bool audio_bgm_play_or_restart(BGM *bgm, bool loop, double position, double fadein) {
	if(bgm == NULL) {
		return B.bgm_stop(0);
	}

	if(B.bgm_play(bgm, loop, position, fadein)) {
		assert(B.bgm_current() == bgm);
		events_emit(TE_AUDIO_BGM_STARTED, 0, bgm, NULL);
		return true;
	}

	return false;
}

bool audio_bgm_stop(double fadeout) {
	return B.bgm_stop(fadeout);
}

bool audio_bgm_pause(void) {
	return B.bgm_pause();
}

bool audio_bgm_resume(void) {
	return B.bgm_resume();
}

double audio_bgm_seek(double pos) {
	return B.bgm_seek(pos);
}

double audio_bgm_seek_realtime(double rtpos) {
	BGM *bgm = audio_bgm_current();

	if(bgm == NULL) {
		log_warn("No BGM is playing");
		return -1;
	}

	double seekpos;
	double duration = bgm_get_duration(bgm);

	if(audio_bgm_looping()) {
		double loop_start = bgm_get_loop_start(bgm);
		seekpos = audioutil_loopaware_position(rtpos, duration, loop_start);
	} else {
		seekpos = clamp(rtpos, 0, duration);
	}

	return audio_bgm_seek(seekpos);
}

double audio_bgm_tell(void) {
	return B.bgm_tell();
}

BGMStatus audio_bgm_status(void) {
	return B.bgm_status();
}

bool audio_bgm_looping(void) {
	return B.bgm_looping();
}

BGM *audio_bgm_current(void) {
	return B.bgm_current();
}
