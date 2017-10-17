/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "global.h"
#include "plrmodes.h"
#include "marisa.h"

static void marisa_laser_draw(Projectile *p, int t) {
    if(REF(p->args[1]) == NULL)
        return;

    if(cimag(p->pos) - cimag(global.plr.pos) < 90) {
        float s = resources.fbo.fg[0].scale;
        glScissor(0, s * (VIEWPORT_H - cimag(((Enemy *)REF(p->args[1]))->pos)), VIEWPORT_W*s, VIEWPORT_H*s);
        glEnable(GL_SCISSOR_TEST);
    }

    ProjDraw(p, t);

    glDisable(GL_SCISSOR_TEST);
}

static int marisa_laser(Projectile *p, int t) {
    if(t == EVENT_DEATH) {
        free_ref(p->args[1]);
        return 1;
    }

    if(REF(p->args[1]) == NULL)
        return ACTION_DESTROY;

    float angle = creal(p->args[2]);
    float factor = (1-global.plr.focus/30.0) * !!angle;
    complex dir = -cexp(I*((angle+0.125*sin(global.frames/25.0)*(angle > 0? 1 : -1))*factor + M_PI/2));
    p->args[0] = 20*dir;
    linear(p, t);

    p->pos = ((Enemy *)REF(p->args[1]))->pos + p->pos;

    return 1;
}

static int marisa_laser_slave(Enemy *e, int t) {
    if(player_should_shoot(&global.plr, true)) {
        if(!(global.frames % 4))
            create_projectile_p(&global.projs, get_tex("proj/marilaser"), 0, 0, marisa_laser_draw, marisa_laser, 0, add_ref(e),e->args[2],0)->type = PlrProj+e->args[1]*4;

        if(!(global.frames % 3)) {
            float s = 0.5 + 0.3*sin(global.frames/7.0);
            create_particle3c("marilaser_part0", 0, rgb(1-s,0.5,s), PartDraw, marisa_laser, 0, add_ref(e), e->args[2])->type=PlrProj;
        }
        create_particle1c("lasercurve", e->pos, 0, Fade, timeout, 4)->type = PlrProj;
    }

    e->pos = global.plr.pos + (1 - global.plr.focus/30.0)*e->pos0 + (global.plr.focus/30.0)*e->args[0];

    return 1;
}

static void masterspark_ring_draw(complex base, int t, float fade) {
    glPushMatrix();

    if(t < 1) {
        // prevent division by zero
        t = 1;
    }

    glTranslatef(creal(base), cimag(base)-t*t*0.4, 0);

    float f = sqrt(t/500.0)*1200;

    glColor4f(1,1,1,fade*20.0/t);

    Texture *tex = get_tex("masterspark_ring");
    glScalef(f/tex->w, 1-tex->h/f,0);
    draw_texture_p(0,0,tex);

    glPopMatrix();
}

static void masterspark_draw(Enemy *e, int t) {
    t = player_get_bomb_progress(&global.plr, NULL) * (e->args[0] / BOMB_RECOVERY);

    glPushMatrix();

    float angle = 9 - t/e->args[0]*6.0, fade = 1;

    if(t < creal(e->args[0]/6))
        fade = t/e->args[0]*6;

    if(t > creal(e->args[0])/4*3)
        fade = 1-t/e->args[0]*4 + 3;

    glColor4f(1,0.85,1,fade);
    glTranslatef(creal(global.plr.pos), cimag(global.plr.pos), 0);
    glRotatef(-angle,0,0,1);
    draw_texture(0, -450, "masterspark");
    glColor4f(0.85,1,1,fade);
    glRotatef(angle*2,0,0,1);
    draw_texture(0, -450, "masterspark");
    glColor4f(1,1,1,fade);
    glRotatef(-angle,0,0,1);
    draw_texture(0, -450, "masterspark");
    glPopMatrix();

//  glColor4f(0.9,1,1,fade*0.8);
    for(int i = 0; i < 8; i++)
        masterspark_ring_draw(global.plr.pos - 50.0*I, t%20 + 10*i, fade);

    glColor4f(1,1,1,1);
}

static int masterspark(Enemy *e, int t) {
    if(t == EVENT_BIRTH) {
        global.shake_view = 8;
        return 1;
    }

    t = player_get_bomb_progress(&global.plr, NULL) * (e->args[0] / BOMB_RECOVERY);

    if(t >= creal(e->args[0]) || global.frames - global.plr.recovery > 0) {
        global.shake_view = 0;
        return ACTION_DESTROY;
    }

    return 1;
}

static void marisa_laser_bomb(Player *plr) {
    play_sound("bomb_marisa_a");
    create_enemy_p(&plr->slaves, 40.0*I, ENEMY_BOMB, masterspark_draw, masterspark, 280,0,0,0);
}

static void marisa_laser_power(Player *plr, short npow) {
    Enemy *e = plr->slaves, *tmp;
    double dmg = 4;

    if(plr->power / 100 == npow / 100)
        return;

    while(e != 0) {
        tmp = e;
        e = e->next;
        if(tmp->hp == ENEMY_IMMUNE)
            delete_enemy(&plr->slaves, tmp);
    }

    if(npow / 100 == 1) {
        create_enemy_p(&plr->slaves, -40.0*I, ENEMY_IMMUNE, marisa_common_slave_draw, marisa_laser_slave, -40.0*I, dmg, 0, 0);
    }

    if(npow >= 200) {
        create_enemy_p(&plr->slaves, 25-5.0*I, ENEMY_IMMUNE, marisa_common_slave_draw, marisa_laser_slave, 8-40.0*I, dmg,   -M_PI/30, 0);
        create_enemy_p(&plr->slaves, -25-5.0*I, ENEMY_IMMUNE, marisa_common_slave_draw, marisa_laser_slave, -8-40.0*I, dmg,  M_PI/30, 0);
    }

    if(npow / 100 == 3) {
        create_enemy_p(&plr->slaves, -30.0*I, ENEMY_IMMUNE, marisa_common_slave_draw, marisa_laser_slave, -50.0*I, dmg, 0, 0);
    }

    if(npow >= 400) {
        create_enemy_p(&plr->slaves, 17-30.0*I, ENEMY_IMMUNE, marisa_common_slave_draw, marisa_laser_slave, 4-45.0*I, dmg, M_PI/60, 0);
        create_enemy_p(&plr->slaves, -17-30.0*I, ENEMY_IMMUNE, marisa_common_slave_draw, marisa_laser_slave, -4-45.0*I, dmg, -M_PI/60, 0);
    }
}

static double marisa_laser_speed_mod(Player *plr, double speed) {
    if(global.frames - plr->recovery < 0) {
        speed /= 5.0;
    }

    return speed;
}

static void marisa_laser_preload(void) {
    const int flags = RESF_DEFAULT;

    preload_resources(RES_TEXTURE, flags,
        "part/marilaser_part0",
        "proj/marilaser",
        "proj/marisa",
        "masterspark",
        "masterspark_ring",
    NULL);

    preload_resources(RES_SFX, flags | RESF_OPTIONAL,
        "bomb_marisa_a",
    NULL);
}

PlayerMode plrmode_marisa_a = {
    .name = "Laser Sign",
    .character = &character_marisa,
    .shot_mode = PLR_SHOT_MARISA_LASER,
    .procs = {
        .bomb = marisa_laser_bomb,
        .shot = marisa_common_shot,
        .power = marisa_laser_power,
        .speed_mod = marisa_laser_speed_mod,
        .preload = marisa_laser_preload,
    },
};
