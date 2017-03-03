
#include <stdlib.h>
#include <string.h>
#include "bgm.h"

struct current_bgm_t current_bgm;

Sound *load_bgm(const char *filename) {
    return NULL;
}

void start_bgm(const char *name) {
    return;
}

void stop_bgm(void) {
    return;
}

void continue_bgm(void) {
    return;
}

void save_bgm(void) {
    return;
}

void restore_bgm(void) {
    return;
}

int init_bgm(void) {
    memset(&current_bgm, 0, sizeof(struct current_bgm_t));
    return 0;
}

void shutdown_bgm(void) {
    return;
}

void load_bgm_descriptions(const char *path) {
    return;
}

void delete_music(void) {
    return;
}

void set_bgm_volume(float gain) {
    return;
}
