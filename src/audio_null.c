
#include "audio.h"

void audio_backend_init(void) {}
void audio_backend_shutdown(void) {}
bool audio_backend_initialized(void) { return false; }
void audio_backend_set_sfx_volume(float gain) {}
void audio_backend_set_bgm_volume(float gain) {}
bool audio_backend_music_is_paused(void) { return false; }
bool audio_backend_music_is_playing(void) { return false; }
void audio_backend_music_resume(void) { return; }
void audio_backend_music_stop(void) {}
void audio_backend_music_pause(void) {}
bool audio_backend_music_play(void *impl) { return false; }
bool audio_backend_sound_play(void *impl) { return false; }
