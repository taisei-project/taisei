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
		} else 
#ifdef OGG_SUPPORT_ENABLED
		if( (strcmp(dp->d_name + strlen(dp->d_name)-4, ".wav") == 0)
		 || (strcmp(dp->d_name + strlen(dp->d_name)-4, ".ogg") == 0))
#else
		if  (strcmp(dp->d_name + strlen(dp->d_name)-4, ".wav") == 0)
#endif
		{
			// BGMs should have "bgm_" prefix!
			if(strncmp(dp->d_name, "bgm_", 4) == 0)
				load_bgm(buf, dp->d_name + strlen(dp->d_name)-3);
			else
				load_sound(buf, dp->d_name + strlen(dp->d_name)-3);
		} else if(strcmp(dp->d_name + strlen(dp->d_name)-4, ".sha") == 0) {
			load_shader_file(buf);
		} else if(strcmp(dp->d_name + strlen(dp->d_name)-4, ".obj") == 0) {
			load_model(buf);
		}
		
		free(buf);
	}
	
	closedir(dir);
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
	
	if(!tconfig.intval[NO_AUDIO] && !(resources.state & RS_SfxLoaded)) {
		printf("- sounds:\n");
		strcpy(path, get_prefix());
		strcat(path, "sfx");
		recurse_dir(path);
		
		resources.state |= RS_SfxLoaded;
	}
	
	if(!tconfig.intval[NO_MUSIC] && !(resources.state & RS_BgmLoaded)) {
		printf("- music:\n");
		strcpy(path, get_prefix());
		strcat(path, "bgm");
		load_bgm_descriptions(path);
		recurse_dir(path);
		
		resources.state |= RS_BgmLoaded;
	}
	
	if(!tconfig.intval[NO_SHADER] && !(resources.state & RS_ShaderLoaded)) {
		printf("- shader:\n");
		strcpy(path, get_prefix());
		strcat(path, "shader");
		recurse_dir(path);
		
		strcpy(path, get_prefix());
		strcat(path, "shader/laser_snippets");
		
		if(tgl_ext[TGLEXT_draw_instanced])
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
}

void free_resources(void) {	
	// Music and sounds are freed by corresponding functions
	
	printf("-- freeing textures\n");
	delete_textures();
	
	printf("-- freeing models\n");
	delete_animations();
	
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
}

void draw_loading_screen(void) {
	const char *prefix = get_prefix();
	char *buf = malloc(strlen(prefix)+16);
	Texture *tex;
	
	strcpy(buf, prefix);
	strcat(buf, "gfx/loading.png");
	
	set_ortho();
	
	tex = load_texture(buf);
	
	draw_texture_p(SCREEN_W/2,SCREEN_H/2, tex);
	SDL_GL_SwapBuffers();
}