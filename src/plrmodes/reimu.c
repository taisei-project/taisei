/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "global.h"
#include "plrmodes.h"
#include "reimu.h"
#include "recolor.h"

PlayerCharacter character_reimu = {
    .id = PLR_CHAR_REIMU,
    .lower_name = "reimu",
    .proper_name = "Reimu",
    .full_name = "Hakurei Reimu",
    .title = "Shrine Maiden",
    .dialog_sprite_name = "dialog/reimu",
    .player_sprite_name = "reimu",
    .ending = {
        .good = good_ending_marisa,
        .bad = bad_ending_marisa,
    },
};

void reimu_yinyang_visual(Enemy *e, int t, bool render) {
    if(!render) {
        return;
    }

    Shader *s = recolor_get_shader();
    Color c = rgb(0.95, 0.75, 1.0);
    float b = 1.0;
    ColorTransform ct = {
        .R[1] = rgba(-0.5, -0.5, -0.5, 0.0),
        .G[1] = c & ~CLRMASK_A,
        .B[1] = rgba(b, b, b, 0),
        .A[1] = c &  CLRMASK_A,
    };

    glUseProgram(s->prog);
    recolor_apply_transform(&ct);

    glPushMatrix();
    glTranslatef(creal(e->pos), cimag(e->pos), -1);
    glRotatef(global.frames * 6, 0, 0, 1);
    draw_texture_with_size(0, 0, 24, 24, "yinyang");
    glPopMatrix();
    glUseProgram(0);
}
