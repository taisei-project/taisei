/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "difficulty.h"
#include "resource/resource.h"

const char* difficulty_name(Difficulty diff) {
    switch(diff) {
        case D_Easy:    return "Easy";      break;
        case D_Normal:  return "Normal";    break;
        case D_Hard:    return "Hard";      break;
        case D_Lunatic: return "Lunatic";   break;
        case D_Extra:   return "Extra";     break;
        default:        return "Unknown";   break;
    }
}

const char* difficulty_tex(Difficulty diff) {
    switch(diff) {
        case D_Easy:    return "difficulty/easy";
        case D_Normal:  return "difficulty/normal";
        case D_Hard:    return "difficulty/hard";
        case D_Lunatic: return "difficulty/lunatic";
        case D_Extra:   return "difficulty/lunatic";
	default: return "difficulty/unknown"; // This texture is not supposed to exist.
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
        preload_resource(RES_TEXTURE, difficulty_tex(diff), RESF_PERMANENT);
    }
}
