/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

#include "resource/sfx.h"
#include "resource/bgm.h"

#define LOOPTIMEOUTFRAMES 10
#define DEFAULT_SFX_VOLUME 100

#define LOOPFADEOUT 300
// XXX: THIS IS SPARTA. but this needs to be a property set in volumes.conf or wherever

typedef struct CurrentBGM {
	char *name;
	char *title;
	int isboss;
	int started_at;
	Music *music;
} CurrentBGM;

extern CurrentBGM current_bgm;

typedef enum {
	SNDGROUP_ALL,
	SNDGROUP_MAIN,
	SNDGROUP_UI,
} AudioBackendSoundGroup;

void audio_backend_init(void);
void audio_backend_shutdown(void);
bool audio_backend_initialized(void);
void audio_backend_set_sfx_volume(float gain);
void audio_backend_set_bgm_volume(float gain);
bool audio_backend_music_is_paused(void);
bool audio_backend_music_is_playing(void);
void audio_backend_music_resume(void);
void audio_backend_music_stop(void);
void audio_backend_music_fade(double fadetime);
void audio_backend_music_pause(void);
bool audio_backend_music_play(void *impl);
bool audio_backend_music_set_position(double pos);
bool audio_backend_sound_play(void *impl, AudioBackendSoundGroup group);
bool audio_backend_sound_play_or_restart(void *impl, AudioBackendSoundGroup group);
bool audio_backend_sound_loop(void *impl, AudioBackendSoundGroup group);
bool audio_backend_sound_stop_loop(void *impl);
bool audio_backend_sound_pause_all(AudioBackendSoundGroup group);
bool audio_backend_sound_resume_all(AudioBackendSoundGroup group);
bool audio_backend_sound_stop_all(AudioBackendSoundGroup group);

void audio_init(void);
void audio_shutdown(void);

void play_sound(const char *name) attr_nonnull(1);
void play_sound_ex(const char *name, int cooldown, bool replace) attr_nonnull(1);
void play_sound_delayed(const char *name, int cooldown, bool replace, int delay) attr_nonnull(1);
void play_loop(const char *name) attr_nonnull(1);
void play_ui_sound(const char *name) attr_nonnull(1);
void reset_sounds(void);
void pause_sounds(void);
void resume_sounds(void);
void stop_sounds(void);
void update_sounds(void); // checks if loops need to be stopped

int get_default_sfx_volume(const char *sfx);

Sound* get_sound(const char *name) attr_nonnull(1);
Music* get_music(const char *music) attr_nonnull(1);

void start_bgm(const char *name);
void stop_bgm(bool force);
void fade_bgm(double fadetime);
void resume_bgm(void);
void save_bgm(void); // XXX: this is broken
void restore_bgm(void); // XXX: this is broken
