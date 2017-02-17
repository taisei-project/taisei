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
#include "bgm.h"

#ifdef OGG_SUPPORT_ENABLED
#include <vorbis/vorbisfile.h>
#include "ogg.h"
#endif

struct current_bgm_t current_bgm = { .name = NULL };

char *saved_bgm;

int init_bgm(int *argc, char *argv[])
{
	if (config_get_int(CONFIG_NO_MUSIC)) return 1;
	if (!init_alut_if_needed(argc, argv)) return 0;

	alGenSources(1, &resources.bgmsrc);
	if(warn_alut_error("creating music sources"))
	{
		config_set_int(CONFIG_NO_MUSIC, 1);
		unload_alut_if_needed();
		return 0;
	}
	return 1;
}

void shutdown_bgm(void)
{
	current_bgm.name = NULL;
	alDeleteSources(1, &resources.bgmsrc);
	warn_alut_error("deleting music sources");
	if(resources.state & RS_BgmLoaded)
	{
		printf("-- freeing music\n");
		delete_music();
		resources.state &= ~RS_BgmLoaded;
	}
	config_set_int(CONFIG_NO_MUSIC, 1);
	unload_alut_if_needed();
}

char *get_bgm_desc(Bgm_desc *source, char *name) {
	for(; source; source = source->next)
		if(strcmp(source->name, name) == 0)
			return source->value;
	return NULL;
}

void delete_bgm_description(void **descs, void *desc) {
	free(((Bgm_desc *)desc)->name);
	free(((Bgm_desc *)desc)->value);
	delete_element(descs, desc);
}

void load_bgm_descriptions(const char *path) {
	const char *fname = "/bgm.conf";

	char *fullname = malloc(strlen(path)+strlen(fname)+1);
	if (fullname == NULL) return;
	strcpy(fullname, path);
	strcat(fullname, fname);

	FILE *fp = fopen(fullname, "rt");
	free(fullname);
	if (fp == NULL) return;

	char line[256];
	while(fgets(line, sizeof(line), fp))
	{
		char *rem;
		while ((rem = strchr(line,'\n')) != NULL) *rem='\0';
		while ((rem = strchr(line,'\r')) != NULL) *rem='\0';
		while ((rem = strchr(line,'\t')) != NULL) *rem=' ';
		if    ((rem = strchr(line,' ' )) == NULL)
		{
			if (strlen(line) > 0)
				warnx("load_bgm_description():\n!- illegal string format. See README.");
			continue;
		}

		*(rem++)='\0';
		Bgm_desc *desc = create_element((void **)&resources.bgm_descriptions, sizeof(Bgm_desc));

		desc->name  = strdup(line);
		desc->value = malloc(strlen(rem) + 6);
		if (!desc->name || !desc->value)
		{
			delete_bgm_description((void**)resources.bgm_descriptions, desc);
			warnx("load_bgm_description():\n!- strdup() failed");
			continue;
		}
		strcpy(desc->value, "BGM: ");
		strcat(desc->value, rem);
		printf ("Music %s is now known as \"%s\".\n", desc->name, desc->value);
	}

	fclose(fp);
	return;
}

Sound *load_bgm(char *filename, const char *type) {
	return load_sound_or_bgm(filename, &resources.music, "bgm/", type);
}

void start_bgm(char *name) {
	if(config_get_int(CONFIG_NO_MUSIC)) return;

	if(!name || strcmp(name, "") == 0)
	{
		stop_bgm();
		return;
	}

	// if BGM has changed, change it and start from beginning
	if (!current_bgm.name || strcmp(name, current_bgm.name))
	{
		current_bgm.name = realloc(current_bgm.name, strlen(name) + 1);

		if(current_bgm.name == NULL)
			errx(-1,"start_bgm():\n!- realloc error with music '%s'", name);

		strcpy(current_bgm.name, name);
		if((current_bgm.data = get_snd(resources.music, name)) == NULL)
		{
			warnx("start_bgm():\n!- BGM '%s' not exist", current_bgm.name);
			stop_bgm();
			free(current_bgm.name);
			current_bgm.name = NULL;
			return;
		}
		alSourceRewind(resources.bgmsrc);
		warn_alut_error("rewinding music source");
		alSourcei(resources.bgmsrc, AL_BUFFER, current_bgm.data->alsnd);
		warn_alut_error("changing buffer of music source");
		alSourcei(resources.bgmsrc, AL_LOOPING, AL_TRUE);
		warn_alut_error("looping music source");
	}

	// otherwise, do not change anything and continue
	ALint play;
	alGetSourcei(resources.bgmsrc,AL_SOURCE_STATE,&play);
	warn_alut_error("checking state of music source");

	// Support drawing BGM title in game loop
	if ((current_bgm.title = get_bgm_desc(resources.bgm_descriptions, current_bgm.name)) != NULL)
	{
		current_bgm.started_at = global.frames;
		// Boss BGM title color may differ from the one at beginning of stage
		current_bgm.isboss = (strstr(current_bgm.name, "boss") != NULL);
	}
	else
	{
		current_bgm.started_at = -1;
	}

	if(play != AL_PLAYING)
	{
		alSourcePlay(resources.bgmsrc);
		warn_alut_error("starting playback of music source");
		printf("Started %s\n", (current_bgm.title ? current_bgm.title : current_bgm.name));
	}
}

void continue_bgm(void)
{
	start_bgm(current_bgm.name);
}

void stop_bgm(void) {
	if (config_get_int(CONFIG_NO_MUSIC) || !current_bgm.name) return;

	ALint play;
	alGetSourcei(resources.bgmsrc,AL_SOURCE_STATE,&play);
	warn_alut_error("checking state of music source");
	if(play == AL_PLAYING)
	{
		alSourcePause(resources.bgmsrc);
		warn_alut_error("pausing music source");
		printf("BGM stopped.\n");
	}
	else
	{
		printf("stop_bgm(): No BGM was playing.\n");
	}
}

void save_bgm(void)
{
	free(saved_bgm); // Deal with consequent saves without restore.
	saved_bgm = current_bgm.name ? strdup(current_bgm.name) : NULL;
}

void restore_bgm(void)
{
	start_bgm(saved_bgm);
	free(saved_bgm);
	saved_bgm = NULL;
}

void set_bgm_volume(float gain)
{
	if(config_get_int(CONFIG_NO_MUSIC)) return;
	printf("BGM volume: %f\n", gain);
	alSourcef(resources.bgmsrc,AL_GAIN, gain);
	warn_alut_error("changing gain of music source");
}

void delete_music(void) {
	delete_all_elements((void **)&resources.bgm_descriptions, delete_bgm_description);
	delete_all_elements((void **)&resources.music, delete_sound);
}
