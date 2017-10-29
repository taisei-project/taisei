/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "audio.h"

void audio_backend_init(void) {}
void audio_backend_shutdown(void) {}
bool audio_backend_initialized(void) { return false; }
void audio_backend_set_sfx_volume(float gain) {}
void audio_backend_set_bgm_volume(float gain) {}
bool audio_backend_music_is_paused(void) { return false; }
bool audio_backend_music_is_playing(void) { return false; }
void audio_backend_music_resume(void) {}
void audio_backend_music_stop(void) {}
void audio_backend_music_fade(double fadetime) {}
void audio_backend_music_pause(void) {}
bool audio_backend_music_play(void *impl) { return false; }
bool audio_backend_sound_play_or_restart(void *impl, AudioBackendSoundGroup group) { return false; }
bool audio_backend_music_set_position(double pos) { return false; }
bool audio_backend_sound_play(void *impl, AudioBackendSoundGroup group) { return false; }
bool audio_backend_sound_loop(void *impl, AudioBackendSoundGroup group) { return false; }
bool audio_backend_sound_stop_loop(void *impl) { return false; }
bool audio_backend_sound_pause_all(AudioBackendSoundGroup group) { return false; }
bool audio_backend_sound_resume_all(AudioBackendSoundGroup group) { return false; }
bool audio_backend_sound_stop_all(AudioBackendSoundGroup group) { return false; }
