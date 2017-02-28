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

// Should be global to be possibly used in free_resources():
/* bool vbo_set_up = false; */ // should be set to true in init_quadfbo() ?
bool fbo_set_up = false;
bool fonts_set_up = false;

void recurse_dir(char *path, bool transient)
{
	// Exit if we load permanent resources, but 'path' is related to transient ones.
	// 'path' should be relative under get_prefix().
	if (!transient && strstr(path, "stage"))
	{
		warnx("Skipped transient directory '%s': we load permanent resources.", path);
		return;
	}

	char *real_path = malloc(strlen(get_prefix()) + strlen(path) + 1);
	if(real_path == NULL)
		errx(-1, "Unable to allocate real_path (prefix='%s', path='%s')", get_prefix(), path);

	strcpy(real_path, get_prefix());
	strcat(real_path, path);

	DIR *dir = opendir(real_path);
	if(dir == NULL)
		errx(-1, "Can't open directory '%s%s'", get_prefix(), path);

	struct dirent *dp;

	char *audio_exts[] = { ".wav", ".ogg", ".mp3", ".mod", ".xm", ".s3m",
		".669", ".it", ".med", ".mid", ".flac", ".aiff", ".voc",
		NULL };

	while((dp = readdir(dir)) != NULL)
	{
		// For load_...() functions: they require absolute path
		char *real_buf = malloc(strlen(real_path) + strlen(dp->d_name)+2);
		strcpy(real_buf, real_path);
		strcat(real_buf, "/");
		strcat(real_buf, dp->d_name);

		// For recurse_dir(): it requires relative path (to determine transiency)
		char *buf = malloc(strlen(path) + strlen(dp->d_name)+2);
		strcpy(buf, path);
		strcat(buf, "/");
		strcat(buf, dp->d_name);

		struct stat statbuf;
		stat(real_buf, &statbuf);

		if(S_ISDIR(statbuf.st_mode) && dp->d_name[0] != '.')
		{
			// Recurse any directory (no matter, permanent or transient)
			recurse_dir(buf, transient);
		}
		else
		{
			bool perform_loading = true;
			
			// Don't load anything if we load transient resources, but 'path' is not related to it.
			// Note: we can skip only files, not dirs, because dirs may contain transient subdirs.
			if(transient)
			{
				if (!global.stage)
					errx(-1, "Trying to load transient resources with uninitialized global.stage.");
				if (!global.stage->resource_id)
					errx(-1, "Trying to load transient resources with uninitialized global.stage->resource_id.");
				
				char stagepath[32];
				sprintf(stagepath, "stage%d", global.stage->resource_id);
				
				if(!strstr(path, stagepath)) perform_loading = false;
			}
			
			if(perform_loading)
			{
				if(strendswith(dp->d_name, ".png"))
				{
					if(strncmp(dp->d_name, "ani_", 4) == 0)
						init_animation(real_buf, transient);
					else
						load_texture(real_buf, transient);
				}
				else if(strendswith(dp->d_name, ".sha"))
				{
					load_shader_file(real_buf, transient);
				}
				else if(strendswith(dp->d_name, ".obj"))
				{
					load_model(real_buf, transient);
				}
				else
				{
					int i;
					for (i = 0; audio_exts[i] != NULL; i++)
					{
						if (strendswith(dp->d_name, audio_exts[i]))
						{
							// BGMs should have "bgm_" prefix!
							if(strncmp(dp->d_name, "bgm_", 4) == 0)
								load_bgm(real_buf, transient);
							else
								load_sound(real_buf, transient);
							break;
						}
					}
				}
			}
		}
		
		free(buf);
		free(real_buf);
	}

	free(real_path);
	closedir(dir);
}

static void resources_cfg_noshader_callback(ConfigIndex idx, ConfigValue v) {
	config_set_int(idx, v.i);

	if(!v.i) {
		load_resources(false);
	}
}

void load_resources(bool transient) {
	static bool callbacks_set_up = false;

	if(!callbacks_set_up) {
		config_set_callback(CONFIG_NO_SHADER, resources_cfg_noshader_callback);
		callbacks_set_up = true;
	}

	printf("Loading %s resources:\n", transient ? "transient" : "permanent");

	printf("- textures:\n");
	recurse_dir("gfx", transient);

	if(!config_get_int(CONFIG_NO_AUDIO)) {
		printf("- sounds:\n");
		recurse_dir("sfx", transient);
	}

	if(!config_get_int(CONFIG_NO_MUSIC)) {
		printf("- music:\n");
		if (!transient) load_bgm_descriptions("bgm");
		recurse_dir("bgm", transient);
	}

	if(!config_get_int(CONFIG_NO_SHADER)) {
		printf("- shader:\n");
		recurse_dir("shader", transient);

		if(!transient && glext.draw_instanced)
		{
			// Snippets are permanent (for now).
			const char *snippet_filename = "shader/laser_snippets"; // Should we make it a #define?
			char *path = malloc(strlen(get_prefix()) + strlen(snippet_filename) + 1);
			if (path == NULL)
				errx(-1, "A problem while constructing snippet filename.");
			strcpy(path, get_prefix());
			strcat(path, snippet_filename);
			load_shader_snippets(path, "laser_");
		}

		if (!fbo_set_up)
		{
			printf("init_fbo():\n");
			init_fbo(&resources.fbg[0]);
			init_fbo(&resources.fbg[1]);
			init_fbo(&resources.fsec);
			printf("-- finished\n");
			fbo_set_up = true;
		}
	}

	printf("- models:\n");
	recurse_dir("models", transient);

	if(!fonts_set_up)
	{
		printf("- fonts:\n");
		init_fonts();
		fonts_set_up = true;
	}
}

// Freeing permanent resources should be called only once at exit;
// it's the limitation of freeing VBO.
void free_resources(bool transient) {
	printf("Freeing %s resources: ", transient ? "transient" : "permanent");

	printf("SFX and BGM");
	delete_sounds(transient);
	delete_music(transient);

	// Will shut down only if ALL music and/or SFX deleted
	shutdown_sfx();
	shutdown_bgm();

	printf(", textures");
	delete_textures(transient);

	printf(", models");
	delete_animations(transient);

	if(/* vbo_set_up & */ !transient)
	{
		printf(", VBOs");
		delete_vbo(&_vbo);
		/* vbo_set_up = false; */
	}

	if(fbo_set_up & !transient) {
		printf(", FBOs");
		delete_fbo(&resources.fbg[0]);
		delete_fbo(&resources.fbg[1]);
		delete_fbo(&resources.fsec);
		fbo_set_up = false;
	}

	printf(", shaders");
	delete_shaders(transient);

	if(fonts_set_up & !transient)
	{
		printf(", fonts");
		// Should we deinit fonts? do_something() here, if so...
		// fonts_set_up = false; // still commented as we do not deinit fonts at all
	}

	printf(".\n");
}

void draw_loading_screen(void) {
	const char *prefix = get_prefix();
	char *buf = malloc(strlen(prefix)+16);
	Texture *tex;

	strcpy(buf, prefix);
	strcat(buf, "gfx/loading.png");

	set_ortho();

	tex = load_texture(buf, false);
	free(buf);

	draw_texture_p(SCREEN_W/2,SCREEN_H/2, tex);
	SDL_GL_SwapWindow(video.window);
}
