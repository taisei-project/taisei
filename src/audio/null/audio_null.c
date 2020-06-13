/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "../backend.h"
#include "util.h"

static bool audio_null_init(void) {
	if(strcmp(env_get("TAISEI_AUDIO_BACKEND", ""), "null")) { // don't warn if we asked for this
		log_warn("Audio subsystem initialized with the 'null' backend. There will be no sound");
	}

	return true;
}

#define FAKE_SOUND_ID ((SFXPlayID)1)

static BGM *audio_null_bgm_current(void) { return NULL; }
static double audio_null_bgm_seek(double pos) { return -1; }
static double audio_null_bgm_tell(void) { return -1; }
static BGMStatus audio_null_bgm_status(void) { return BGM_STOPPED; }
static bool audio_null_bgm_looping(void) { return false; }
static bool audio_null_bgm_pause(void) { return false; }
static bool audio_null_bgm_play(BGM *bgm, bool loop, double pos, double fadein) { return false; }
static bool audio_null_bgm_resume(void) { return false; }
static bool audio_null_bgm_set_global_volume(double gain) { return false; }
static bool audio_null_bgm_stop(double fadeout) { return false; }
static bool audio_null_output_works(void) { return false; }
static bool audio_null_shutdown(void) { return true; }
static SFXPlayID audio_null_sfx_loop(SFXImpl *impl, AudioBackendSFXGroup group) { return FAKE_SOUND_ID; }
static bool audio_null_sfx_pause_all(AudioBackendSFXGroup group) { return true; }
static SFXPlayID audio_null_sfx_play(SFXImpl *impl, AudioBackendSFXGroup group) { return FAKE_SOUND_ID; }
static SFXPlayID audio_null_sfx_play_or_restart(SFXImpl *impl, AudioBackendSFXGroup group) { return FAKE_SOUND_ID; }
static bool audio_null_sfx_resume_all(AudioBackendSFXGroup group) { return true; }
static bool audio_null_sfx_set_global_volume(double gain) { return true; }
static bool audio_null_sfx_set_volume(SFXImpl *snd, double gain) { return true; }
static bool audio_null_sfx_stop_all(AudioBackendSFXGroup group) { return true; }
static bool audio_null_sfx_stop_loop(SFXImpl *impl) { return true; }
static bool audio_null_sfx_stop_id(SFXPlayID sid) { return true; }

static const char* const* audio_null_get_supported_exts(uint *out_numexts) { *out_numexts = 0; return NULL; }
static BGM *audio_null_bgm_load(const char *vfspath) { return NULL; }
static SFXImpl *audio_null_sfx_load(const char *vfspath) { return NULL; }
static void audio_null_bgm_unload(BGM *mus) { }
static void audio_null_sfx_unload(SFXImpl *snd) { }

static const char *audio_null_obj_bgm_get_title(BGM *bgm) { return NULL; }
static const char *audio_null_obj_bgm_get_artist(BGM *bgm) { return NULL; }
static const char *audio_null_obj_bgm_get_comment(BGM *bgm) { return NULL; }
static double audio_null_obj_bgm_get_duration(BGM *bgm) { return 1; }
static double audio_null_obj_bgm_get_loop_start(BGM *bgm) {	return 0; }

AudioBackend _a_backend_null = {
	.name = "null",
	.funcs = {
		.bgm_current = audio_null_bgm_current,
		.bgm_load = audio_null_bgm_load,
		.bgm_looping = audio_null_bgm_looping,
		.bgm_pause = audio_null_bgm_pause,
		.bgm_play = audio_null_bgm_play,
		.bgm_resume = audio_null_bgm_resume,
		.bgm_seek = audio_null_bgm_seek,
		.bgm_set_global_volume = audio_null_bgm_set_global_volume,
		.bgm_status = audio_null_bgm_status,
		.bgm_stop = audio_null_bgm_stop,
		.bgm_tell = audio_null_bgm_tell,
		.bgm_unload = audio_null_bgm_unload,
		.get_supported_bgm_exts = audio_null_get_supported_exts,
		.get_supported_sfx_exts = audio_null_get_supported_exts,
		.init = audio_null_init,
		.output_works = audio_null_output_works,
		.sfx_load = audio_null_sfx_load,
		.sfx_loop = audio_null_sfx_loop,
		.sfx_pause_all = audio_null_sfx_pause_all,
		.sfx_play = audio_null_sfx_play,
		.sfx_play_or_restart = audio_null_sfx_play_or_restart,
		.sfx_resume_all = audio_null_sfx_resume_all,
		.sfx_set_global_volume = audio_null_sfx_set_global_volume,
		.sfx_set_volume = audio_null_sfx_set_volume,
		.sfx_stop_all = audio_null_sfx_stop_all,
		.sfx_stop_id = audio_null_sfx_stop_id,
		.sfx_stop_loop = audio_null_sfx_stop_loop,
		.sfx_unload = audio_null_sfx_unload,
		.shutdown = audio_null_shutdown,

		.object = {
			.bgm = {
				.get_title = audio_null_obj_bgm_get_title,
				.get_artist = audio_null_obj_bgm_get_artist,
				.get_comment = audio_null_obj_bgm_get_comment,
				.get_duration = audio_null_obj_bgm_get_duration,
				.get_loop_start = audio_null_obj_bgm_get_loop_start,
			},
		},
	}
};
