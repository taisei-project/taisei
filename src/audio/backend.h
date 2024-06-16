/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "audio.h"

typedef int16_t AudioBackendChannel;
#define AUDIO_BACKEND_CHANNEL_INVALID ((AudioBackendChannel)-1)

typedef struct AudioBGMObjectFuncs {
	const char *(*get_title)(BGM *bgm);
	const char *(*get_artist)(BGM *bgm);
	const char *(*get_comment)(BGM *bgm);
	double (*get_duration)(BGM *bgm);
	double (*get_loop_start)(BGM *bgm);
} AudioBGMObjectFuncs;

typedef struct AudioSFXObjectFuncs {
	bool (*set_volume)(SFXImpl *sfx, double vol);
} AudioSFXObjectFuncs;

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
	bool (*output_works)(void);
	bool (*shutdown)(void);
	AudioBackendChannel (*sfx_play)(SFXImpl *sfx, AudioChannelGroup group, AudioBackendChannel chan, bool loop);
	bool (*chan_stop)(AudioBackendChannel chan, double fadeout);
	bool (*chan_unstop)(AudioBackendChannel chan, double fadein);
	const char *const *(*get_supported_bgm_exts)(uint *out_numexts);
	const char *const *(*get_supported_sfx_exts)(uint *out_numexts);
	BGM *(*bgm_load)(const char *vfspath);
	SFXImpl *(*sfx_load)(const char *vfspath);
	void (*bgm_unload)(BGM *bgm);
	void (*sfx_unload)(SFXImpl *sfx);
	bool (*group_get_channels_range)(AudioChannelGroup group, int *first, int *last);
	bool (*group_pause)(AudioChannelGroup group);
	bool (*group_resume)(AudioChannelGroup group);
	bool (*group_set_volume)(AudioChannelGroup group, double vol);
	bool (*group_stop)(AudioChannelGroup group, double fadeout);

	struct {
		AudioBGMObjectFuncs bgm;
		AudioSFXObjectFuncs sfx;
	} object;
} AudioFuncs;

typedef struct AudioBackend {
	const char *name;
	AudioFuncs funcs;
	void *custom;
} AudioBackend;

extern AudioBackend _a_backend;

bool audio_backend_init(void);
