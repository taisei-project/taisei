/*
 * This software is licensed under the terms of the MIT-License
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

static struct {
	ResourceRef current_bgm;
	ht_str2int_t sfx_volumes;
	struct enqueued_sound {
		LIST_INTERFACE(struct enqueued_sound);
		char *name;
		int time;
		int cooldown;
		bool replace;
	} *sound_queue;
} audio;

static void play_sound_internal(const char *name, bool is_ui, int cooldown, bool replace, int delay) {
	if(!audio_output_works()) {
		return;
	}

	if(delay > 0) {
		struct enqueued_sound *s = malloc(sizeof(struct enqueued_sound));
		s->time = global.frames + delay;
		s->name = strdup(name);
		s->cooldown = cooldown;
		s->replace = replace;
		list_push(&audio.sound_queue, s);
		return;
	}

	if(global.frameskip) {
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
	play_sound_internal(snd->name, false, snd->cooldown, snd->replace, 0);
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

static void* reset_sounds_callback(const char *name, void *res, void *arg) {
	bool reset = (intptr_t)arg;
	Sound *snd = res;

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

	return NULL;
}

void reset_sounds(void) {
	res_for_each(RES_SOUND, reset_sounds_callback, (void*)true);
	list_foreach(&audio.sound_queue, discard_enqueued_sound, NULL);
}

void update_sounds(void) {
	res_for_each(RES_SOUND, reset_sounds_callback, (void*)false);

	for(struct enqueued_sound *s = audio.sound_queue, *next; s; s = next) {
		next = (struct enqueued_sound*)s->next;

		if(s->time <= global.frames) {
			play_enqueued_sound(&audio.sound_queue, s, NULL);
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
	return res_get_data(RES_SOUND, name, RESF_OPTIONAL);
}

Music* get_music(const char *name) {
	return res_get_data(RES_MUSIC, name, RESF_OPTIONAL);
}

static bool store_sfx_volume(const char *key, const char *val, void *data) {
	int vol = atoi(val);

	if(vol < 0 || vol > 128) {
		log_warn("Volume %i for sfx %s is out of range; must be within [0, 128]", vol, key);
		vol = vol < 0 ? 0 : 128;
	}

	log_debug("Default volume for %s is now %i", key, vol);

	if(vol != DEFAULT_SFX_VOLUME) {
		ht_set(&audio.sfx_volumes, key, vol);
	}

	return true;
}

static void load_config_files(void) {
	ht_create(&audio.sfx_volumes);
	const char *conf_name = "volumes.conf";
	const char *prefix = res_type_pathprefix(RES_SOUND);
	char buf[strlen(prefix) + strlen(conf_name) + 2];
	snprintf(buf, sizeof(buf), "%s" VFS_PATH_SEPARATOR_STR "%s", prefix, conf_name);
	parse_keyvalue_file_cb(buf, store_sfx_volume, NULL);
}

int get_default_sfx_volume(const char *sfx) {
	return ht_get(&audio.sfx_volumes, sfx, DEFAULT_SFX_VOLUME);
}

bool bgm_get_ref(ResourceRef *out_ref, bool weak) {
	if(res_ref_is_valid(audio.current_bgm)) {
		if(out_ref) {
			*out_ref = (weak ? res_ref_copy_weak : res_ref_copy)(audio.current_bgm);
		}

		return true;
	} else {
		return false;
	}
}

static void stop_bgm_internal(bool pause, double fadetime) {
	if(!res_ref_is_valid(audio.current_bgm)) {
		log_debug("No BGM was playing (no ref)");
		return;
	}

	if(!pause) {
		res_unref(&audio.current_bgm, 1);
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

void bgm_stop(bool force) {
	stop_bgm_internal(!force, 0);
}

void bgm_fade(double fadetime) {
	stop_bgm_internal(false, fadetime);
}

static bool is_playing_specific_bgm(ResourceRef bgmref) {
	return
		res_ref_is_valid(audio.current_bgm) &&
		res_refs_are_equivalent(audio.current_bgm, bgmref);
}

const char *bgm_get_title(void) {
	if(res_ref_is_valid(audio.current_bgm)) {
		Music *mus = res_ref_data(audio.current_bgm);

		if(mus == NULL) {
			return NULL;
		}

		MusicMetadata *meta = res_ref_data(mus->meta);

		if(meta != NULL) {
			return meta->title;
		}
	}

	return NULL;
}

static void start_bgm_internal(ResourceRef bgmref) {
	assert(res_ref_status(bgmref) != RES_STATUS_EXTERNAL);

	// if BGM has changed, change it and start from beginning
	if(!is_playing_specific_bgm(bgmref)) {
		// TODO: perhaps make this async
		if(res_ref_wait_ready(bgmref) == RES_STATUS_FAILED) {
			log_error("BGM '%s' could not be loaded", res_ref_name(bgmref));
			return;
		}

		B.music_stop();
		res_unref_if_valid(&audio.current_bgm, 1);
		bgmref = res_ref_copy(bgmref);
		audio.current_bgm = bgmref;
	}

	if(B.music_is_paused()) {
		B.music_resume();
	}

	if(B.music_is_playing()) {
		return;
	}

	if(!B.music_play(((Music*)res_ref_data(bgmref))->impl)) {
		log_error("BGM '%s' could not start playing", res_ref_name(bgmref));
		bgm_stop(true);
		return;
	}

	log_info("Started %s", bgm_get_title());
}

void bgm_start(const char *name) {
	if(!name || !*name) {
		bgm_stop(false);
		return;
	}

	ResourceRef bgmref = res_ref(RES_MUSIC, name, RESF_OPTIONAL);
	start_bgm_internal(bgmref);
	res_unref(&bgmref, 1);
}

void bgm_start_ref(ResourceRef bgmref) {
	start_bgm_internal(bgmref);
}

void bgm_resume(void) {
	if(res_ref_is_valid(audio.current_bgm)) {
		start_bgm_internal(audio.current_bgm);
	}
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
	res_unref_if_valid(&audio.current_bgm, 1);
	events_unregister_handler(audio_config_updated);
	B.shutdown();
	ht_destroy(&audio.sfx_volumes);
}

bool audio_output_works(void) {
	return B.output_works();
}

void audio_music_set_position(double pos) {
	B.music_set_position(pos);
}
