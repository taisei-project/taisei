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

struct current_bgm_t current_bgm = { .name = NULL };

char *saved_bgm;

static void bgm_cfg_nomusic_callback(ConfigIndex idx, ConfigValue v) {
	config_set_int(idx, v.i);

	if(v.i) {
		shutdown_bgm();
		return;
	}

	if(!init_bgm()) {
		config_set_int(idx, true);
		return;
	}

	load_resources();
	set_bgm_volume(config_get_float(CONFIG_BGM_VOLUME));
	start_bgm("bgm_menu"); // FIXME: start BGM that's supposed to be playing in the current context
}

static void bgm_cfg_volume_callback(ConfigIndex idx, ConfigValue v) {
	set_bgm_volume(config_set_float(idx, v.f));
}

int init_bgm(void)
{
	static bool callbacks_set_up = false;

	if(!callbacks_set_up) {
		config_set_callback(CONFIG_NO_MUSIC, bgm_cfg_nomusic_callback);
		config_set_callback(CONFIG_BGM_VOLUME, bgm_cfg_volume_callback);
		callbacks_set_up = true;
	}

	if (config_get_int(CONFIG_NO_MUSIC)) return 1;
	if (!init_mixer_if_needed()) return 0;
	return 1;
}

void shutdown_bgm(void)
{
	current_bgm.name = NULL;
	if(resources.state & RS_BgmLoaded)
	{
		printf("-- freeing music\n");
		delete_music();
		resources.state &= ~RS_BgmLoaded;
	}
	unload_mixer_if_needed();
}

char *get_bgm_desc(char *name) {
	return (char*)hashtable_get_string(resources.bgm_descriptions, name);
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

		char *value = malloc(strlen(rem) + 6);
		strcpy(value, "BGM: ");
		strcat(value, rem);

		hashtable_set_string(resources.bgm_descriptions, line, value);

		printf("Music %s is now known as \"%s\".\n", line, value);
	}

	fclose(fp);
	return;
}

Sound *load_bgm(const char *filename) {
	return load_sound_or_bgm(filename, resources.music, ST_MUSIC);
}

void start_bgm(const char *name) {
	if(config_get_int(CONFIG_NO_MUSIC)) return;

	// start_bgm(NULL) or start_bgm("") would be equivalent to stop_bgm().
	if(!name || strcmp(name, "") == 0)
	{
		stop_bgm();
		return;
	}

	// if BGM has changed, change it and start from beginning
	if (!current_bgm.name || strcmp(name, current_bgm.name))
	{
		Mix_HaltMusic();

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
	}

	if(Mix_PausedMusic()) Mix_ResumeMusic(); // Unpause music if paused
	if(Mix_PlayingMusic()) return; // Do nothing if music already playing (or was just unpaused)

	if(Mix_PlayMusic(current_bgm.data->music, -1) == -1) // Start playing otherwise
		printf("Failed starting BGM %s: %s.\n", current_bgm.name, Mix_GetError());
	// Support drawing BGM title in game loop (only when music changed!)
	if ((current_bgm.title = get_bgm_desc(current_bgm.name)) != NULL)
	{
		current_bgm.started_at = global.frames;
		// Boss BGM title color may differ from the one at beginning of stage
		current_bgm.isboss = (strstr(current_bgm.name, "boss") != NULL);
	}
	else
	{
		current_bgm.started_at = -1;
	}

	printf("Started %s\n", (current_bgm.title ? current_bgm.title : current_bgm.name));
}

void continue_bgm(void)
{
	start_bgm(current_bgm.name); // In most cases it's just unpausing existing music.
}

void stop_bgm(void) {
	if (config_get_int(CONFIG_NO_MUSIC) || !current_bgm.name) return;

	if(Mix_PlayingMusic() && !Mix_PausedMusic())
	{
		Mix_PauseMusic(); // Pause, not halt - to be unpaused in continue_bgm() if needed.
		printf("BGM stopped.\n");
	}
	else
	{
		printf("stop_bgm(): No BGM was playing.\n");
	}
}

void save_bgm(void)
{
	// Deal with consequent saves without restore.
	stralloc(&saved_bgm, current_bgm.name);
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
	Mix_VolumeMusic(gain * MIX_MAX_VOLUME);
}

void delete_music(void) {
	resources_delete_and_unset_all(resources.bgm_descriptions, hashtable_iter_free_data, NULL);
	resources_delete_and_unset_all(resources.music, delete_sound, NULL);
}
