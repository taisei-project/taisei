/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2012, Alexeyew Andrew <http://akari.thebadasschoobs.org/>
 */

#include "difficulty.h"

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
        case D_Easy:    return "difficulty/easy";      break;
        case D_Normal:  return "difficulty/normal";    break;
        case D_Hard:    return "difficulty/hard";      break;
        case D_Lunatic: return "difficulty/lunatic";   break;
        case D_Extra:   return "difficulty/lunatic";     break;
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
