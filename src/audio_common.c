/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2012, Alexeyew Andrew <http://akari.thebadasschoobs.org/>
 */

#include <string.h>
#include <stdio.h>

#include "audio.h"
#include "resource/resource.h"
#include "global.h"

static char *saved_bgm;
static Hashtable *bgm_descriptions;
CurrentBGM current_bgm = { .name = NULL };

static void play_sound_internal(const char *name, bool unconditional) {
    if(!audio_backend_initialized() || global.frameskip) {
        return;
    }

    Sound *snd = get_sound(name);

    if(!snd || (!unconditional && snd->lastplayframe == global.frames)) {
        return;
    }

    snd->lastplayframe = global.frames;
    audio_backend_sound_play(snd->impl);
}

void play_sound(const char *name) {
    play_sound_internal(name, false);
}

void play_ui_sound(const char *name) {
    play_sound_internal(name, true);
}

void reset_sounds(void) {
    Resource *snd;
    for(HashtableIterator *i = hashtable_iter(resources.handlers[RES_SFX].mapping);
        hashtable_iter_next(i, NULL, (void**)&snd);) {
        snd->sound->lastplayframe = 0;
    }
}

Sound* get_sound(const char *name) {
    Resource *res = get_resource(RES_SFX, name, RESF_OPTIONAL);
    return res ? res->sound : NULL;
}

Music* get_music(const char *name) {
    Resource *res = get_resource(RES_BGM, name, RESF_OPTIONAL);
    return res ? res->music : NULL;
}

static void sfx_cfg_volume_callback(ConfigIndex idx, ConfigValue v) {
    audio_backend_set_sfx_volume(config_set_float(idx, v.f));
}

static void bgm_cfg_volume_callback(ConfigIndex idx, ConfigValue v) {
    audio_backend_set_bgm_volume(config_set_float(idx, v.f));
}

static void load_bgm_descriptions(void) {
    char *fullname = strjoin(get_prefix(), "bgm/bgm.conf", NULL);
    FILE *fp = fopen(fullname, "rt");
    free(fullname);

    bgm_descriptions = hashtable_new_stringkeys(16);

    if(fp == NULL) {
        return;
    }

    char line[256];
    while(fgets(line, sizeof(line), fp)) {
        char *rem;

        while((rem = strchr(line,'\n')) != NULL) *rem = '\0';
        while((rem = strchr(line,'\r')) != NULL) *rem = '\0';
        while((rem = strchr(line,'\t')) != NULL) *rem = ' ';

        if((rem = strchr(line,' ' )) == NULL) {
            if(strlen(line) > 0)
                warnx("load_bgm_description(): illegal string format. See README.");
            continue;
        }

        *(rem++)='\0';

        char *value = strjoin("BGM: ", rem, NULL);
        hashtable_set_string(bgm_descriptions, line, value);
        printf("Music %s is now known as \"%s\".\n", line, value);
    }

    fclose(fp);
    return;
}

static inline char* get_bgm_desc(char *name) {
    return (char*)hashtable_get_string(bgm_descriptions, name);
}

void resume_bgm(void) {
    start_bgm(current_bgm.name); // In most cases it just unpauses existing music.
}

void stop_bgm(bool force) {
    if(!current_bgm.name) {
        return;
    }

    if(audio_backend_music_is_playing() && !audio_backend_music_is_paused()) {
        if(force) {
            audio_backend_music_stop();
        } else {
            audio_backend_music_pause();
        }

        printf("BGM stopped.\n");
    } else {
        printf("stop_bgm(): No BGM was playing.\n");
    }
}

void save_bgm(void) {
    // Deal with consequent saves without restore.
    stralloc(&saved_bgm, current_bgm.name);
}

void restore_bgm(void) {
    start_bgm(saved_bgm);
    free(saved_bgm);
    saved_bgm = NULL;
}

void start_bgm(const char *name) {
    if(!name || !*name) {
        stop_bgm(false);
        return;
    }

    // if BGM has changed, change it and start from beginning
    if(!current_bgm.name || strcmp(name, current_bgm.name)) {
        audio_backend_music_stop();

        stralloc(&current_bgm.name, name);

        if((current_bgm.music = get_music(name)) == NULL) {
            warnx("start_bgm(): BGM '%s' does not exist", current_bgm.name);
            stop_bgm(true);
            free(current_bgm.name);
            current_bgm.name = NULL;
            return;
        }
    }

    if(audio_backend_music_is_paused()) {
        audio_backend_music_resume();
    }

    if(audio_backend_music_is_playing()) {
        return;
    }

    if(!audio_backend_music_play(current_bgm.music->impl)) {
        return;
    }

    // Support drawing BGM title in game loop (only when music changed!)
    if((current_bgm.title = get_bgm_desc(current_bgm.name)) != NULL) {
        current_bgm.started_at = global.frames;
        // Boss BGM title color may differ from the one at beginning of stage
        current_bgm.isboss = strendswith(current_bgm.name, "boss");
    } else {
        current_bgm.started_at = -1;
    }

    printf("Started %s\n", (current_bgm.title ? current_bgm.title : current_bgm.name));
}

void audio_init(void) {
    audio_backend_init();
    load_bgm_descriptions();
    config_set_callback(CONFIG_SFX_VOLUME, sfx_cfg_volume_callback);
    config_set_callback(CONFIG_BGM_VOLUME, bgm_cfg_volume_callback);
}

void audio_shutdown(void) {
    audio_backend_shutdown();
    hashtable_foreach(bgm_descriptions, hashtable_iter_free_data, NULL);
    hashtable_free(bgm_descriptions);
}
