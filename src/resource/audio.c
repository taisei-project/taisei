/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "audio.h"
#include "resource.h"
#include "global.h"
#include "list.h"
#include "taisei_err.h"

#if defined (OGG_FOUND)
#include <ogg/ogg.h>
#endif

struct current_bgm_t current_bgm = { .name = NULL };

Sound *load_sound_or_bgm(char *filename, Sound **dest, const char *res_directory) {
	ALuint sound;
	if(!(sound = alutCreateBufferFromFile(filename)))
		errx(-1,"load_sound():\n!- cannot load '%s%s': %s", res_directory, filename, alutGetErrorString(alutGetError()));
	
	Sound *snd = create_element((void **)dest, sizeof(Sound));
	
	snd->alsnd = sound;
	snd->lastplayframe = 0;
	
	char *beg = strstr(filename, res_directory) + 4;
	char *end = strrchr(filename, '.');
	
	snd->name = malloc(end - beg + 1);
	memset(snd->name, 0, end-beg + 1);
	strncpy(snd->name, beg, end-beg);
	
	printf("-- loaded '%s%s' as '%s'\n", res_directory, filename, snd->name);
	
	return snd;
}

Sound *load_bgm(char *filename) {
	return load_sound_or_bgm(filename, &resources.music, "bgm/");
}

void start_bgm(char *name) {
	if(!tconfig.intval[NO_MUSIC])
	{
		// if BGM has changed, change it and start from beginning
		if (!current_bgm.name || !strcmp(name, current_bgm.name))
		{
			stop_bgm();
			current_bgm.name = realloc(current_bgm.name, strlen(name) + 1);
			
			if(current_bgm.name == NULL)
				errx(-1,"start_bgm():\n!- realloc error with music '%s'", name);
			
			strcpy(current_bgm.name, name);
			current_bgm.data = get_snd(resources.music, name);
			alSourcei(resources.bgmsrc, AL_BUFFER, current_bgm.data->alsnd);
			alSourcei(resources.bgmsrc, AL_LOOPING, AL_TRUE);
			alSourceRewind(resources.bgmsrc);
		}
		else // otherwise, do not change anything and continue
		{
			alSourcePlay(resources.bgmsrc);
		}
	}
}

void stop_bgm() {
	if (current_bgm.name)
	{
		ALint play;
		alGetSourcei(resources.bgmsrc,AL_SOURCE_STATE,&play);
		if(play == AL_PLAYING)
		{
			alSourcePause(resources.bgmsrc);
		}
	}
}

Sound *load_sound(char *filename) {
	return load_sound_or_bgm(filename, &resources.sounds, "sfx/");
}

Sound *get_snd(Sound *source, char *name) {
	Sound *s, *res = NULL;
	for(s = source; s; s = s->next) {
		if(strcmp(s->name, name) == 0)
			res = s;
	}
	
	if(res == NULL)
		errx(-1,"get_snd():\n!- cannot load sound '%s'", name);
	
	return res;
}

void play_sound(char *name) {
	if(!tconfig.intval[NO_AUDIO])
		play_sound_p(get_snd(resources.sounds, name));
}

void play_sound_p(Sound *snd) {
	if(tconfig.intval[NO_AUDIO] || snd->lastplayframe == global.frames)
		return;
	
	snd->lastplayframe = global.frames;
	
	ALuint i,res = -1;
	ALint play;
	for(i = 0; i < SNDSRC_COUNT; i++) {
		alGetSourcei(resources.sndsrc[i],AL_SOURCE_STATE,&play);
		if(play != AL_PLAYING) {
			res = i;
			break;
		}
	}
	
	if(res != -1) {
		alSourcei(resources.sndsrc[res],AL_BUFFER, snd->alsnd);
		alSourcePlay(resources.sndsrc[res]);
	} else {
		warnx("play_sound_p():\n!- not enough sources");
	}
}

void delete_sound(void **snds, void *snd) {
	free(((Sound *)snd)->name);
	alDeleteBuffers(1, &((Sound *)snd)->alsnd);
	delete_element(snds, snd);
}

void delete_sounds(void) {
	delete_all_elements((void **)&resources.sounds, delete_sound);
}

void delete_music(void) {
	delete_all_elements((void **)&resources.music, delete_sound);
}
