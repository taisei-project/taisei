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

#define LOOPTIMEOUTFRAMES 10
#define DEFAULT_SFX_VOLUME 100
#define SFX_STOP_FADETIME 0.15
#define SFX_LOOPSTOP_FADETIME 0.05
#define SFX_LOOPUNSTOP_FADETIME 0.02

struct SFX {
	SFXImpl *impl;
	int lastplayframe;
	bool looping;

	struct {
		SFXPlayID last_loop_id;
		SFXPlayID last_play_id;
	} per_group[NUM_SFX_CHANGROUPS];
};

#define B (_a_backend.funcs)

struct enqueued_sound {
	LIST_INTERFACE(struct enqueued_sound);
	char *name;
	int time;
	int cooldown;
	bool replace;
};

static struct {
	ht_str2int_t sfx_volumes;
	struct enqueued_sound *sound_queue;
	uint32_t *chan_play_ids;
	uint32_t play_counter;
	int sfx_chan_first, sfx_chan_last;
} audio;

static inline int sfx_chanidx(AudioBackendChannel ch) {
	int i = (int)ch - audio.sfx_chan_first;
	assume(i >= 0);
	assume(i <= audio.sfx_chan_last);
	return i;
}

#define ASSERT_SFX_CHAN_VALID(ch) ((void)sfx_chanidx(ch))

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
	parse_keyvalue_file_cb(SFX_PATH_PREFIX "volumes.conf", store_sfx_volume, NULL);
}

static bool audio_config_updated(SDL_Event *evt, void *arg) {
	if(config_get_int(CONFIG_MUTE_AUDIO)) {
		B.group_set_volume(CHANGROUP_SFX_GAME, 0.0);
		B.group_set_volume(CHANGROUP_SFX_UI, 0.0);
		B.group_set_volume(CHANGROUP_BGM, 0.0);
	} else {
		B.group_set_volume(CHANGROUP_SFX_GAME, config_get_float(CONFIG_SFX_VOLUME));
		B.group_set_volume(CHANGROUP_SFX_UI, config_get_float(CONFIG_SFX_VOLUME));
		B.group_set_volume(CHANGROUP_BGM, config_get_float(CONFIG_BGM_VOLUME));
	}

	return false;
}

static bool get_chanrange(AudioChannelGroup g) {
	int cfirst, clast;

	if(UNLIKELY(!B.group_get_channels_range(g, &cfirst, &clast))) {
		return false;
	}

	assume(clast >= cfirst);

	audio.sfx_chan_first = imin(audio.sfx_chan_first, cfirst);
	audio.sfx_chan_last = imax(audio.sfx_chan_last, clast);

	return true;
}

void audio_init(void) {
	load_config_files();
	audio_backend_init();
	events_register_handler(&(EventHandler) {
		audio_config_updated, NULL, EPRIO_SYSTEM, MAKE_TAISEI_EVENT(TE_CONFIG_UPDATED)
	});
	audio_config_updated(NULL, NULL);

	audio.sfx_chan_first = INT_MAX;
	audio.sfx_chan_last = INT_MIN;

	bool have_chans = false;

	// NOTE: assuming game and UI channels are contiguous for simplicity.
	// If they happen to be not, the code should still work, it'll just waste some memory.
	have_chans |= get_chanrange(CHANGROUP_SFX_GAME);
	have_chans |= get_chanrange(CHANGROUP_SFX_UI);

	if(LIKELY(have_chans)) {
		int num_chans = audio.sfx_chan_last - audio.sfx_chan_first + 1;
		assume(num_chans > 0);
		audio.chan_play_ids = calloc(num_chans, sizeof(*audio.chan_play_ids));
	}
}

void audio_shutdown(void) {
	events_unregister_handler(audio_config_updated);
	B.shutdown();
	ht_destroy(&audio.sfx_volumes);
}

bool audio_output_works(void) {
	return B.output_works();
}

SFX *audio_sfx_load(const char *name, const char *path) {
	SFXImpl *impl = B.sfx_load(path);

	if(UNLIKELY(!impl)) {
		return NULL;
	}

	double vol = ht_get(&audio.sfx_volumes, name, DEFAULT_SFX_VOLUME) / 128.0;
	B.object.sfx.set_volume(impl, vol);

	SFX *sfx = calloc(1, sizeof(*sfx));
	sfx->impl = impl;
	return sfx;
}

void audio_sfx_destroy(SFX *sfx) {
	B.sfx_unload(sfx->impl);
	free(sfx);
}

static bool is_skip_mode(void) {
	return global.frameskip || stage_is_skip_mode();
}

INLINE SFXPlayID pack_playid(AudioChannelGroup chan, uint32_t play_id) {
	ASSERT_SFX_CHAN_VALID(chan);
	return ((uint32_t)chan + 1) | ((uint64_t)play_id << 32ull);
}

INLINE void unpack_playid(SFXPlayID pid, AudioBackendChannel *chan, uint32_t *play_id) {
	int ch = (int)((uint32_t)(pid & 0xffffffff) - 1);
	ASSERT_SFX_CHAN_VALID(ch);
	*chan = ch;
	*play_id = pid >> 32ull;
}

INLINE AudioBackendChannel get_playid_chan(SFXPlayID play_id) {
	if(play_id == 0) {
		return AUDIO_BACKEND_CHANNEL_INVALID;
	}

	AudioBackendChannel chan;
	uint32_t counter;
	unpack_playid(play_id, &chan, &counter);

	if(audio.chan_play_ids[sfx_chanidx(chan)] == counter) {
		return chan;
	}

	return AUDIO_BACKEND_CHANNEL_INVALID;
}

static SFXPlayID register_sfx_playback(
	SFX *sfx, AudioChannelGroup g, AudioBackendChannel ch, bool loop
) {;
	uint32_t cntr = ++audio.play_counter;
	audio.chan_play_ids[sfx_chanidx(ch)] = cntr;
	SFXPlayID play_id = pack_playid(ch, cntr);

	if(loop) {
		sfx->per_group[g].last_loop_id = play_id;
	} else {
		sfx->per_group[g].last_play_id = play_id;
	}

	sfx->lastplayframe = global.frames;
	return play_id;
}

static SFXPlayID submit_play_sfx(
	SFX *sfx, AudioChannelGroup g, AudioBackendChannel ch, bool loop
) {
	ch = B.sfx_play(sfx->impl, g, ch, loop);

	if(UNLIKELY(ch == AUDIO_BACKEND_CHANNEL_INVALID)) {
		return 0;
	}

	return register_sfx_playback(sfx, g, ch, loop);
}

static SFXPlayID play_sound_internal(
	const char *name, bool is_ui, int cooldown, bool replace, int delay
) {
	if(!audio_output_works() || is_skip_mode()) {
		return 0;
	}

	if(delay > 0) {
		struct enqueued_sound *s = malloc(sizeof(struct enqueued_sound));
		s->time = global.frames + delay;
		s->name = strdup(name);
		s->cooldown = cooldown;
		s->replace = replace;
		list_push(&audio.sound_queue, s);
		return 0;
	}

	SFX *sfx = res_sfx(name);

	if(!sfx || (!is_ui && sfx->lastplayframe + 3 + cooldown >= global.frames)) {
		return 0;
	}

	sfx->lastplayframe = global.frames;

	AudioChannelGroup group = is_ui ? CHANGROUP_SFX_UI : CHANGROUP_SFX_GAME;
	AudioBackendChannel ch = AUDIO_BACKEND_CHANNEL_INVALID;

	if(replace) {
		SFXPlayID id = sfx->per_group[group].last_play_id;
		ch = get_playid_chan(id);

		if(ch != AUDIO_BACKEND_CHANNEL_INVALID) {
			group = CHANGROUP_ANY;
		}
	}

	return submit_play_sfx(sfx, group, ch, false);
}

static void *discard_enqueued_sound(List **queue, List *vsnd, void *arg) {
	struct enqueued_sound *snd = (struct enqueued_sound*)vsnd;
	free(snd->name);
	free(list_unlink(queue, vsnd));
	return NULL;
}

static void *play_enqueued_sound(struct enqueued_sound **queue, struct enqueued_sound *snd, void *arg) {
	if(!is_skip_mode()) {
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

static void stop_sfx_fadeout(SFXPlayID sid, double fadeout) {
	AudioBackendChannel ch = get_playid_chan(sid);

	if(ch != AUDIO_BACKEND_CHANNEL_INVALID) {
		B.chan_stop(ch, fadeout);
	}
}

void stop_sfx(SFXPlayID sid) {
	stop_sfx_fadeout(sid, SFX_STOP_FADETIME);
}

void replace_sfx(SFXPlayID sid, const char *name) {
	stop_sfx(sid);
	play_sfx(name);
}

void play_sfx_loop(const char *name) {
	if(!audio_output_works() || is_skip_mode()) {
		return;
	}

	SFX *sfx = res_sfx(name);

	if(!sfx) {
		return;
	}

	sfx->lastplayframe = global.frames;

	if(sfx->looping) {
		return;
	}

	sfx->looping = true;

	// If a previous loop is fading out, try to quickly fade it back in.
	// Otherwise, start a new loop.

	SFXPlayID sid = sfx->per_group[CHANGROUP_SFX_GAME].last_loop_id;
	AudioBackendChannel ch = get_playid_chan(sid);

	if(ch == AUDIO_BACKEND_CHANNEL_INVALID || !B.chan_unstop(ch, SFX_LOOPUNSTOP_FADETIME)) {
		submit_play_sfx(sfx, CHANGROUP_SFX_GAME, AUDIO_BACKEND_CHANNEL_INVALID, true);
	}
}

static void stop_sfx_loop(SFX *sfx, double fadeout) {
	SFXPlayID sid = sfx->per_group[CHANGROUP_SFX_GAME].last_loop_id;
	AudioBackendChannel ch = get_playid_chan(sid);

	if(ch != AUDIO_BACKEND_CHANNEL_INVALID) {
		B.chan_stop(ch, fadeout);
	}
}

static void *update_sounds_callback(const char *name, Resource *res, void *arg) {
	bool reset = (intptr_t)arg;
	SFX *sfx = res->data;

	if(UNLIKELY(!sfx)) {
		return NULL;
	}

	if(sfx->looping && (reset || global.frames > sfx->lastplayframe + LOOPTIMEOUTFRAMES)) {
		stop_sfx_loop(sfx, SFX_LOOPSTOP_FADETIME);
		sfx->looping = false;
	}

	if(reset) {
		sfx->lastplayframe = 0;
	}

	return NULL;
}

void reset_all_sfx(void) {
	resource_for_each(RES_SFX, update_sounds_callback, (void*)true);
	list_foreach(&audio.sound_queue, discard_enqueued_sound, NULL);
}

void update_all_sfx(void) {
	resource_for_each(RES_SFX, update_sounds_callback, (void*)false);

	for(struct enqueued_sound *s = audio.sound_queue, *next; s; s = next) {
		next = (struct enqueued_sound*)s->next;

		if(s->time <= global.frames) {
			play_enqueued_sound(&audio.sound_queue, s, NULL);
		}
	}
}

void pause_all_sfx(void) {
	B.group_pause(CHANGROUP_SFX_GAME);
}

void resume_all_sfx(void) {
	B.group_resume(CHANGROUP_SFX_GAME);
}

void stop_all_sfx(void) {
	B.group_stop(CHANGROUP_SFX_GAME, 0);
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
