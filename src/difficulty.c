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

void difficulty_color(Color *c, Difficulty diff) {
    switch(diff) {
        case D_Easy:    c->r = 0.5; c->g = 1.0; c->b = 0.5; break;
        case D_Normal:  c->r = 0.5; c->g = 0.5; c->b = 1.0; break;
        case D_Hard:    c->r = 1.0; c->g = 0.5; c->b = 0.5; break;
        case D_Lunatic: c->r = 1.0; c->g = 0.5; c->b = 1.0; break;
        case D_Extra:   c->r = 0.5; c->g = 1.0; c->b = 1.0; break;
        default:        c->r = 0.5; c->g = 0.5; c->b = 0.5; break;
    }
}
