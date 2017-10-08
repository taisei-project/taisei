/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "global.h"
#include "plrmodes.h"

PlayerCharacter character_marisa = {
    .id = PLR_CHAR_MARISA,
    .lower_name = "marisa",
    .proper_name = "Marisa",
    .full_name = "Kirisame Marisa",
    .title = "Black Magician",
    .dialog_sprite_name = "dialog/marisa",
    .player_sprite_name = "marisa",
    .ending = {
        .good = good_ending_marisa,
        .bad = bad_ending_marisa,
    },
};

void marisa_common_shot(Player *plr) {
    if(!(global.frames % 4)) {
        play_sound("generic_shot");
    }

    if(!(global.frames % 6)) {
        create_projectile1c("marisa", plr->pos + 10 - 15.0*I, 0, linear, -20.0*I)->type = PlrProj+175;
        create_projectile1c("marisa", plr->pos - 10 - 15.0*I, 0, linear, -20.0*I)->type = PlrProj+175;
    }
}

void marisa_common_slave_draw(Enemy *e, int t) {
    glPushMatrix();
    glTranslatef(creal(e->pos), cimag(e->pos), -1);
    glRotatef(global.frames * 3, 0, 0, 1);
    draw_texture(0,0,"part/lasercurve");
    glPopMatrix();
}
