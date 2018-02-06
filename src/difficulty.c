/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "difficulty.h"
#include "resource/resource.h"

const char* difficulty_name(Difficulty diff) {
	switch(diff) {
		case D_Easy:    return "Easy";
		case D_Normal:  return "Normal";
		case D_Hard:    return "Hard";
		case D_Lunatic: return "Lunatic";
		case D_Extra:   return "Extra";
		default:        return "Unknown";
	}
}

const char* difficulty_sprite_name(Difficulty diff) {
	switch(diff) {
		case D_Easy:    return "difficulty/easy";
		case D_Normal:  return "difficulty/normal";
		case D_Hard:    return "difficulty/hard";
		case D_Lunatic: return "difficulty/lunatic";
		case D_Extra:   return "difficulty/lunatic";
		default:        return "difficulty/unknown"; // This sprite is not supposed to exist.
	}
}

Color difficulty_color(Difficulty diff) {
	switch(diff) {
		case D_Easy:    return rgb(0.5, 1.0, 0.5);
		case D_Normal:  return rgb(0.5, 0.5, 1.0);
		case D_Hard:    return rgb(1.0, 0.5, 0.5);
		case D_Lunatic: return rgb(1.0, 0.5, 1.0);
		case D_Extra:   return rgb(0.5, 1.0, 1.0);
		default:        return rgb(0.5, 0.5, 0.5);
	}
}

void difficulty_preload(void) {
	for(Difficulty diff = D_Easy; diff < NUM_SELECTABLE_DIFFICULTIES + D_Easy; ++diff) {
		preload_resource(RES_SPRITE, difficulty_sprite_name(diff), RESF_PERMANENT);
	}
}
