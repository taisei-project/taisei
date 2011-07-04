/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "resource.h"
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "paths/native.h"
#include "config.h"
#include "taisei_err.h"

Resources resources;

void recurse_dir(char *path) {
	DIR *dir = opendir(path);
	if(dir == NULL)
		errx(-1, "Can't open directory '%s'", path);
	struct dirent *dp;
	
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
		} else if(strcmp(dp->d_name + strlen(dp->d_name)-4, ".wav") == 0) {
			load_sound(buf);
		} else if(strcmp(dp->d_name + strlen(dp->d_name)-4, ".sha") == 0) {
			load_shader(buf);
		}
		
		free(buf);
	}
}

void load_resources() {
	printf("load_resources():\n");
	memset(&resources, 0, sizeof(Resources));
	
	char *path = malloc(strlen(get_prefix())+7);
	
	if(!(resources.state & RS_GfxLoaded)) {
		printf("- textures:\n");
		strcpy(path, get_prefix());
		strcat(path, "gfx");
		recurse_dir(path);
		
		resources.state |= RS_GfxLoaded;
	}
	
	if(!tconfig.intval[NO_AUDIO] && !(resources.state & RS_SfxLoaded)) {
		printf("- sounds:\n");
		
		alGenSources(SNDSRC_COUNT, resources.sndsrc);
		strcpy(path, get_prefix());
		strcat(path, "sfx");
		recurse_dir(path);
		
		resources.state |= RS_SfxLoaded;
	}
	
	if(!tconfig.intval[NO_SHADER] && !(resources.state & RS_ShaderLoaded)) {
		printf("- shader:\n");
		strcpy(path, get_prefix());
		strcat(path, "shader");
		recurse_dir(path);
		
		printf("init_fbo():\n");
		init_fbo(&resources.fbg);
		init_fbo(&resources.fsec);
		printf("-- finished\n");
		
		resources.state |= RS_ShaderLoaded;
	}
}

void free_resources() {
	delete_textures();
	delete_animations();
	
	if(!tconfig.intval[NO_SHADER])
		delete_shaders();
	
	if(!tconfig.intval[NO_AUDIO]) {
		delete_sounds();
		alutExit();
	}
}