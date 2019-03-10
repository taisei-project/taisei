/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#ifndef IGUARD_audio_audio_h
#define IGUARD_audio_audio_h

#include "taisei.h"

#include "resource/sfx.h"
#include "resource/bgm.h"
#include "resource/bgm_metadata.h"

#define LOOPTIMEOUTFRAMES 10
#define DEFAULT_SFX_VOLUME 100

#define LOOPFADEOUT 50

typedef struct MusicImpl MusicImpl;
typedef struct SoundImpl SoundImpl;

typedef enum {
	LS_OFF,
	LS_LOOPING,
	LS_FADEOUT,
} LoopState;

typedef struct Sound {
	int lastplayframe;
	LoopState islooping;
	void *impl;
} Sound;

typedef struct Music {
	MusicImpl *impl;
	MusicMetadata *meta;
} Music;

typedef struct CurrentBGM {
	char *name;
	char *title;
	int isboss;
	int started_at;
	Music *music;
} CurrentBGM;

extern CurrentBGM current_bgm;

void audio_init(void);
void audio_shutdown(void);
bool audio_output_works(void);
void audio_music_set_position(double pos);

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

#endif // IGUARD_audio_audio_h
