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
	SFXGROUP_ALL,
	SFXGROUP_MAIN,
	SFXGROUP_UI,
} AudioBackendSFXGroup;

typedef struct AudioBGMObjectFuncs {
	const char *(*get_title)(BGM *bgm);
	const char *(*get_artist)(BGM *bgm);
	const char *(*get_comment)(BGM *bgm);
	double (*get_duration)(BGM *bgm);
	double (*get_loop_start)(BGM *bgm);
} AudioBGMObjectFuncs;

typedef struct AudioFuncs {
	bool (*init)(void);
	bool (*bgm_play)(BGM *bgm, bool loop, double position, double fadein);
	bool (*bgm_stop)(double fadeout);
	bool (*bgm_pause)(void);
	bool (*bgm_resume)(void);
	double (*bgm_seek)(double pos);
	double (*bgm_tell)(void);
	BGMStatus (*bgm_status)(void);
	BGM *(*bgm_current)(void);
	bool (*bgm_looping)(void);
	bool (*bgm_set_global_volume)(double gain);
	bool (*output_works)(void);
	bool (*shutdown)(void);
	SFXPlayID (*sfx_loop)(SFXImpl *sfx, AudioBackendSFXGroup group);
	bool (*sfx_pause_all)(AudioBackendSFXGroup group);
	SFXPlayID (*sfx_play)(SFXImpl *sfx, AudioBackendSFXGroup group);
	SFXPlayID (*sfx_play_or_restart)(SFXImpl *sfx, AudioBackendSFXGroup group);
	bool (*sfx_resume_all)(AudioBackendSFXGroup group);
	bool (*sfx_set_global_volume)(double gain);
	bool (*sfx_set_volume)(SFXImpl *sfx, double vol);
	bool (*sfx_stop_all)(AudioBackendSFXGroup group);
	bool (*sfx_stop_loop)(SFXImpl *sfx);
	bool (*sfx_stop_id)(SFXPlayID);
	const char *const *(*get_supported_bgm_exts)(uint *out_numexts);
	const char *const *(*get_supported_sfx_exts)(uint *out_numexts);
	BGM *(*bgm_load)(const char *vfspath);
	SFXImpl *(*sfx_load)(const char *vfspath);
	void (*bgm_unload)(BGM *bgm);
	void (*sfx_unload)(SFXImpl *sfx);

	struct {
		AudioBGMObjectFuncs bgm;
	} object;
} AudioFuncs;

typedef struct AudioBackend {
	const char *name;
	AudioFuncs funcs;
	void *custom;
} AudioBackend;

extern AudioBackend _a_backend;

bool audio_backend_init(void);

#endif // IGUARD_audio_backend_h
