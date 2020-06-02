/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_audio_backend_h
#define IGUARD_audio_backend_h

#include "taisei.h"

#include "audio.h"

typedef enum {
	SNDGROUP_ALL,
	SNDGROUP_MAIN,
	SNDGROUP_UI,
} AudioBackendSoundGroup;

typedef struct AudioFuncs {
	bool (*init)(void);
	bool (*music_fade)(double fadetime);
	bool (*music_is_paused)(void);
	bool (*music_is_playing)(void);
	bool (*music_pause)(void);
	bool (*music_play)(MusicImpl *mus);
	bool (*music_resume)(void);
	bool (*music_set_global_volume)(double gain);
	bool (*music_set_loop_point)(MusicImpl *mus, double pos);
	bool (*music_set_position)(double pos);
	double (*music_get_position)(void);
	bool (*music_stop)(void);
	bool (*output_works)(void);
	bool (*set_sfx_volume)(float gain);
	bool (*shutdown)(void);
	SoundID (*sound_loop)(SoundImpl *snd, AudioBackendSoundGroup group);
	bool (*sound_pause_all)(AudioBackendSoundGroup group);
	SoundID (*sound_play)(SoundImpl *snd, AudioBackendSoundGroup group);
	SoundID (*sound_play_or_restart)(SoundImpl *snd, AudioBackendSoundGroup group);
	bool (*sound_resume_all)(AudioBackendSoundGroup group);
	bool (*sound_set_global_volume)(double gain);
	bool (*sound_set_volume)(SoundImpl *snd, double vol);
	bool (*sound_stop_all)(AudioBackendSoundGroup group);
	bool (*sound_stop_loop)(SoundImpl *snd);
	bool (*sound_stop_id)(SoundID);
	const char* const* (*get_supported_music_exts)(uint *out_numexts);
	const char* const* (*get_supported_sound_exts)(uint *out_numexts);
	MusicImpl* (*music_load)(const char *vfspath);
	SoundImpl* (*sound_load)(const char *vfspath);
	void (*music_unload)(MusicImpl *mus);
	void (*sound_unload)(SoundImpl *snd);
} AudioFuncs;

typedef struct AudioBackend {
	const char *name;
	AudioFuncs funcs;
	void *custom;
} AudioBackend;

extern AudioBackend _a_backend;

bool audio_backend_init(void);

#endif // IGUARD_audio_backend_h
