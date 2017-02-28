
#include <stdlib.h>
#include "audio.h"

Sound* load_sound(char *filename) {
    return NULL;
}

Sound* load_sound_or_bgm(char *filename, Sound **dest, sound_type_t type, bool transient) {
    return NULL;
}

void play_sound_p(char *name, int unconditional) {
    return;
}

Sound* get_snd(Sound *source, char *name) {
    return NULL;
}

void delete_sound(void **snds, void *snd) {
    return;
}

void delete_sounds(bool transient) {
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
