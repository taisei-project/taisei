/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include <dirent.h>
#include "resource.h"
#include <sys/types.h>
#include <sys/stat.h>
#include "paths/native.h"
#include "config.h"
#include "taisei_err.h"
#include "video.h"

Resources resources;

void recurse_dir(char *path) {
	DIR *dir = opendir(path);
	if(dir == NULL)
		errx(-1, "Can't open directory '%s'", path);
	struct dirent *dp;

	char *audio_exts[] = { ".wav", ".ogg", ".mp3", ".mod", ".xm", ".s3m",
		".669", ".it", ".med", ".mid", ".flac", ".aiff", ".voc",
		NULL };

	while((dp = readdir(dir)) != NULL) {
		char *buf = malloc(strlen(path) + strlen(dp->d_name)+2);
		strcpy(buf, path);
		strcat(buf, "/");
		strcat(buf, dp->d_name);

		struct stat statbuf;
		stat(buf, &statbuf);

		if(S_ISDIR(statbuf.st_mode) && dp->d_name[0] != '.') {
			recurse_dir(buf);
		} else if(strcmp(dp->d_name + strlen(dp->d_name)-4, ".png") == 0) {
			if(strncmp(dp->d_name, "ani_", 4) == 0)
				init_animation(buf);
			else
				load_texture(buf);
		} else if(strendswith(dp->d_name, ".sha")) {
				load_shader_file(buf);
		} else if(strendswith(dp->d_name, ".obj")) {
			load_model(buf);
		} else {
			int i;
			for (i = 0; audio_exts[i] != NULL; i++)
			{
				if (strendswith(dp->d_name, audio_exts[i])) {
					// BGMs should have "bgm_" prefix!
					if(strncmp(dp->d_name, "bgm_", 4) == 0)
						load_bgm(buf);
					else
						load_sound(buf);

					break;
				}
			}
		}

		free(buf);
	}

	closedir(dir);
}

static void resources_cfg_noshader_callback(ConfigIndex idx, ConfigValue v) {
	config_set_int(idx, v.i);

	if(!v.i) {
		load_resources();
	}
}

static void init_hashtables(void) {
	// sizes carefully pulled out of my ass to reduce collisions a bit
	resources.textures = hashtable_new_stringkeys(227);
	resources.animations = hashtable_new_stringkeys(23);
	resources.sounds = hashtable_new_stringkeys(16);
	resources.music = hashtable_new_stringkeys(16);
	resources.shaders = hashtable_new_stringkeys(29);
	resources.models = hashtable_new_stringkeys(17);
	resources.bgm_descriptions = hashtable_new_stringkeys(16);
}

static void free_hashtables(void) {
	hashtable_free(resources.textures);
	hashtable_free(resources.animations);
	hashtable_free(resources.sounds);
	hashtable_free(resources.music);
	hashtable_free(resources.shaders);
	hashtable_free(resources.models);
	hashtable_free(resources.bgm_descriptions);
}

void init_resources(void) {
	init_hashtables();
	config_set_callback(CONFIG_NO_SHADER, resources_cfg_noshader_callback);
}

void load_resources(void) {
	printf("load_resources():\n");

	char *path = malloc(strlen(get_prefix())+32);

	if(!(resources.state & RS_GfxLoaded)) {
		printf("- textures:\n");
		strcpy(path, get_prefix());
		strcat(path, "gfx");
		recurse_dir(path);

		resources.state |= RS_GfxLoaded;
	}

	if(!config_get_int(CONFIG_NO_AUDIO) && !(resources.state & RS_SfxLoaded)) {
		printf("- sounds:\n");
		strcpy(path, get_prefix());
		strcat(path, "sfx");
		recurse_dir(path);

		resources.state |= RS_SfxLoaded;
	}

	if(!config_get_int(CONFIG_NO_MUSIC) && !(resources.state & RS_BgmLoaded)) {
		printf("- music:\n");
		strcpy(path, get_prefix());
		strcat(path, "bgm");
		load_bgm_descriptions(path);
		recurse_dir(path);

		resources.state |= RS_BgmLoaded;
	}

	if(!config_get_int(CONFIG_NO_SHADER) && !(resources.state & RS_ShaderLoaded)) {
		printf("- shader:\n");
		strcpy(path, get_prefix());
		strcat(path, "shader");
		recurse_dir(path);

		strcpy(path, get_prefix());
		strcat(path, "shader/laser_snippets");

		if(glext.draw_instanced)
			load_shader_snippets(path, "laser_");

		printf("init_fbo():\n");
		init_fbo(&resources.fbg[0]);
		init_fbo(&resources.fbg[1]);
		init_fbo(&resources.fsec);
		printf("-- finished\n");

		resources.state |= RS_ShaderLoaded;
	}

	if(!(resources.state & RS_ModelsLoaded)) {
		printf("- models:\n");
		strcpy(path, get_prefix());
		strcat(path, "models");
		recurse_dir(path);

		resources.state |= RS_ModelsLoaded;
	}

	if(!(resources.state & RS_FontsLoaded)) {
		printf("- fonts:\n");
		init_fonts();

		resources.state |= RS_FontsLoaded;
	}

	free(path);
}

void free_resources(void) {
	// Music and sounds are freed by corresponding functions

	printf("-- freeing textures\n");
	delete_textures();

	printf("-- freeing animations\n");
	delete_animations();

	printf("-- freeing models\n");
	delete_models();

	printf("-- freeing VBOs\n");
	delete_vbo(&_vbo);

	if(resources.state & RS_ShaderLoaded) {
		printf("-- freeing FBOs\n");
		delete_fbo(&resources.fbg[0]);
		delete_fbo(&resources.fbg[1]);
		delete_fbo(&resources.fsec);

		printf("-- freeing shaders\n");
		delete_shaders();
	}

	free_hashtables();
}

void resources_delete_and_unset_all(Hashtable *ht, HTIterCallback ifunc, void *arg) {
	hashtable_foreach(ht, ifunc, arg);
	hashtable_unset_all(ht);
}

void draw_loading_screen(void) {
	const char *prefix = get_prefix();
	char *buf = malloc(strlen(prefix)+16);
	Texture *tex;

	strcpy(buf, prefix);
	strcat(buf, "gfx/loading.png");

	set_ortho();

	tex = load_texture(buf);
	free(buf);

	draw_texture_p(SCREEN_W/2,SCREEN_H/2, tex);
	SDL_GL_SwapWindow(video.window);
}
