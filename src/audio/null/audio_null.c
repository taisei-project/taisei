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

#define FAKE_SOUND_ID ((SoundID)1)

static bool audio_null_music_fade(double fadetime) { return true; }
static bool audio_null_music_is_paused(void) { return false; }
static bool audio_null_music_is_playing(void) { return false; }
static bool audio_null_music_pause(void) { return true; }
static bool audio_null_music_play(MusicImpl *impl) { return true; }
static bool audio_null_music_resume(void) { return true; }
static bool audio_null_music_set_global_volume(double gain) { return true; }
static bool audio_null_music_set_loop_point(MusicImpl *impl, double pos) { return true; }
static bool audio_null_music_set_position(double pos) { return true; }
static bool audio_null_music_stop(void) { return true; }
static bool audio_null_output_works(void) { return false; }
static bool audio_null_shutdown(void) { return true; }
static SoundID audio_null_sound_loop(SoundImpl *impl, AudioBackendSoundGroup group) { return FAKE_SOUND_ID; }
static bool audio_null_sound_pause_all(AudioBackendSoundGroup group) { return true; }
static SoundID audio_null_sound_play(SoundImpl *impl, AudioBackendSoundGroup group) { return FAKE_SOUND_ID; }
static SoundID audio_null_sound_play_or_restart(SoundImpl *impl, AudioBackendSoundGroup group) { return FAKE_SOUND_ID; }
static bool audio_null_sound_resume_all(AudioBackendSoundGroup group) { return true; }
static bool audio_null_sound_set_global_volume(double gain) { return true; }
static bool audio_null_sound_set_volume(SoundImpl *snd, double gain) { return true; }
static bool audio_null_sound_stop_all(AudioBackendSoundGroup group) { return true; }
static bool audio_null_sound_stop_loop(SoundImpl *impl) { return true; }
static bool audio_null_sound_stop_id(SoundID sid) { return true; }

static const char* const* audio_null_get_supported_exts(uint *out_numexts) { *out_numexts = 0; return NULL; }
static MusicImpl* audio_null_music_load(const char *vfspath) { return NULL; }
static SoundImpl* audio_null_sound_load(const char *vfspath) { return NULL; }
static void audio_null_music_unload(MusicImpl *mus) { }
static void audio_null_sound_unload(SoundImpl *snd) { }

AudioBackend _a_backend_null = {
	.name = "null",
	.funcs = {
		.get_supported_music_exts = audio_null_get_supported_exts,
		.get_supported_sound_exts = audio_null_get_supported_exts,
		.init = audio_null_init,
		.music_fade = audio_null_music_fade,
		.music_is_paused = audio_null_music_is_paused,
		.music_is_playing = audio_null_music_is_playing,
		.music_load = audio_null_music_load,
		.music_pause = audio_null_music_pause,
		.music_play = audio_null_music_play,
		.music_resume = audio_null_music_resume,
		.music_set_global_volume = audio_null_music_set_global_volume,
		.music_set_loop_point = audio_null_music_set_loop_point,
		.music_set_position = audio_null_music_set_position,
		.music_stop = audio_null_music_stop,
		.music_unload = audio_null_music_unload,
		.output_works = audio_null_output_works,
		.shutdown = audio_null_shutdown,
		.sound_load = audio_null_sound_load,
		.sound_loop = audio_null_sound_loop,
		.sound_pause_all = audio_null_sound_pause_all,
		.sound_play = audio_null_sound_play,
		.sound_play_or_restart = audio_null_sound_play_or_restart,
		.sound_resume_all = audio_null_sound_resume_all,
		.sound_set_global_volume = audio_null_sound_set_global_volume,
		.sound_set_volume = audio_null_sound_set_volume,
		.sound_stop_all = audio_null_sound_stop_all,
		.sound_stop_loop = audio_null_sound_stop_loop,
		.sound_stop_id = audio_null_sound_stop_id,
		.sound_unload = audio_null_sound_unload,
	}
};
