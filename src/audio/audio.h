/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "resource/sfx.h"
#include "resource/bgm.h"

typedef struct SFXImpl SFXImpl;

typedef enum {
	CHANGROUP_ANY = -1,

	CHANGROUP_SFX_GAME,
	CHANGROUP_SFX_UI,
	CHANGROUP_BGM,

	NUM_CHANGROUPS,
	NUM_SFX_CHANGROUPS = CHANGROUP_BGM,
} AudioChannelGroup;

typedef uint64_t SFXPlayID;

typedef enum BGMStatus {
	BGM_STOPPED,
	BGM_PLAYING,
	BGM_PAUSED,
} BGMStatus;

void audio_init(void);
void audio_shutdown(void);
bool audio_output_works(void);

bool audio_bgm_play(BGM *bgm, bool loop, double position, double fadein);
bool audio_bgm_play_or_restart(BGM *bgm, bool loop, double position, double fadein);
bool audio_bgm_stop(double fadeout);
bool audio_bgm_pause(void);
bool audio_bgm_resume(void);
double audio_bgm_seek(double pos);
double audio_bgm_seek_realtime(double rtpos);
double audio_bgm_tell(void);
BGMStatus audio_bgm_status(void);
bool audio_bgm_looping(void);
BGM *audio_bgm_current(void);

SFX *audio_sfx_load(const char *name, const char *path) attr_nodiscard attr_nonnull(1, 2);
void audio_sfx_destroy(SFX *sfx) attr_nonnull(1);

bool audio_sfx_set_enabled(bool enabled);

// TODO modernize sfx API

SFXPlayID play_sfx(const char *name) attr_nonnull(1);
SFXPlayID play_sfx_ex(const char *name, int cooldown, bool replace) attr_nonnull(1);
void play_sfx_delayed(const char *name, int cooldown, bool replace, int delay) attr_nonnull(1);
void play_sfx_loop(const char *name) attr_nonnull(1);
void play_sfx_ui(const char *name) attr_nonnull(1);
void stop_sfx(SFXPlayID sid);
void replace_sfx(SFXPlayID sid, const char *name) attr_nonnull(2);
void reset_all_sfx(void);
void pause_all_sfx(void);
void resume_all_sfx(void);
void stop_all_sfx(void);
void update_all_sfx(void); // checks if loops need to be stopped

double audioutil_loopaware_position(double rt_pos, double duration, double loop_start);
