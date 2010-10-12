/*
 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA  02110-1301, USA.
 
 ---
 Copyright (C) 2010, Lukas Weber <laochailan@web.de>
 */

#include "global.h"

Global global;

void init_textures() {
	load_texture(FILE_PREFIX "gfx/proyoumu.png", &global.textures.projwave);
	// load_texture(FILE_PREFIX "gfx/projectiles/ball.png", &global.textures.ball);
	// load_texture(FILE_PREFIX "gfx/projectiles/ball_clr.png", &global.textures.ball_c);
	// load_texture(FILE_PREFIX "gfx/projectiles/rice.png", &global.textures.rice);
	// load_texture(FILE_PREFIX "gfx/projectiles/rice_clr.png", &global.textures.rice_c);
	// load_texture(FILE_PREFIX "gfx/projectiles/bigball.png", &global.textures.bigball);
	// load_texture(FILE_PREFIX "gfx/projectiles/bigball_clr.png", &global.textures.bigball_c);
	load_texture(FILE_PREFIX "gfx/wasser.png", &global.textures.water);
	load_texture(FILE_PREFIX "gfx/hud.png", &global.textures.hud);
	
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, global.textures.water.gltex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glDisable(GL_TEXTURE_2D);
}

void init_global() {
	init_player(&global.plr, Youmu);
	
	init_textures();
	
	global.projs = NULL;
	global.fairies = NULL;
	
	global.frames = 0;
}
