
#include <stdlib.h>
#include "audio.h"

Sound* load_sound(const char *filename) {
    return NULL;
}

Sound* load_sound_or_bgm(const char *filename, Hashtable *ht, sound_type_t type) {
    return NULL;
}

void play_sound_p(const char *name, int unconditional) {
    return;
}

Sound* get_snd(Hashtable *source, const char *name) {
    return NULL;
}

void* delete_sound(void *name, void *snd, void *arg) {
    return NULL;
}

void delete_sounds(void) {
    return;
}

void set_sfx_volume(float gain) {
    return;
}

int init_sfx(void) {
    return 0;
}

void shutdown_sfx(void) {
    return;
}

int init_mixer_if_needed(void) {
    return 0;
}

void unload_mixer_if_needed(void) {
    return;
}
