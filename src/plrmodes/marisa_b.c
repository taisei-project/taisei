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

static void marisa_star(Projectile *p, int t) {
    glPushMatrix();
    glTranslatef(creal(p->pos), cimag(p->pos), 0);
    glRotatef(t*10, 0, 0, 1);
    ProjDrawCore(p, p->color);
    glPopMatrix();
}

static void marisa_star_trail_draw(Projectile *p, int t) {
    float s = 1 - t / creal(p->args[0]);

    Color clr = derive_color(p->color, CLRMASK_A, rgba(0, 0, 0, s*0.5));

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glPushMatrix();
    glTranslatef(creal(p->pos), cimag(p->pos), 0);
    glRotatef(t*10, 0, 0, 1);
    glScalef(s, s, 1);
    ProjDrawCore(p, clr);
    glColor4f(1,1,1,1);
    glPopMatrix();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

static void marisa_star_bomb_draw(Projectile *p, int t) {
    marisa_star(p, t);
}

static int marisa_star_projectile(Projectile *p, int t) {
    float c = 0.3 * psin(t * 0.2);
    p->color = rgb(1 - c, 0.7 + 0.3 * psin(t * 0.1), 0.9 + c/3);

    int r = accelerated(p, t);

    PARTICLE(
        .texture_ptr = get_tex("proj/maristar"),
        .pos = p->pos,
        .color = p->color,
        .rule = timeout,
        .args = { 10 },
        .draw_rule = marisa_star_trail_draw,
        .type = PlrProj,
    );

    if(t == EVENT_DEATH) {
        PARTICLE(
            .texture_ptr = get_tex("proj/maristar"),
            .pos = p->pos,
            .color = p->color,
            .rule = timeout,
            .draw_rule = GrowFade,
            .args = { 40, 2 },
            .type = PlrProj,
            .angle = frand() * 2 * M_PI,
            .flags = PFLAG_DRAWADD,
        );
    }

    return r;
}

static int marisa_star_slave(Enemy *e, int t) {
    double focus = global.plr.focus/30.0;

    if(player_should_shoot(&global.plr, true) && !(global.frames % 15)) {
        complex v = e->args[1] * 2;
        complex a = e->args[2];

        v = creal(v) * (1 - 5 * focus) + I * cimag(v) * (1 - 2.5 * focus);
        a = creal(a) * focus * -0.0525 + I * cimag(a) * 2;

        PROJECTILE(
            .texture_ptr = get_tex("proj/maristar"),
            .pos = e->pos,
            .color = rgb(1.0, 0.5, 1.0),
            .rule = marisa_star_projectile,
            .draw_rule = marisa_star,
            .args = { v, a },
            .type = PlrProj + e->args[3] * 15,
            .color_transform_rule = proj_clrtransform_particle,
        );
    }

    e->pos = global.plr.pos + (1 - focus)*e->pos0 + focus*e->args[0];

    return 1;
}

static int marisa_star_orbit_logic(void *v, int t, double speed) {
    Projectile *p = v;
    float r = cabs(p->pos0 - p->pos);

    p->args[1] = (0.5e5-t*t)*cexp(I*carg(p->pos0 - p->pos))/(r*r);
    p->args[0] += p->args[1]*0.2;
    p->pos += p->args[0];

    PARTICLE("maristar_orbit", p->pos, 0, timeout, { 40 / speed },
        .draw_rule = GrowFade,
        .type = PlrProj,
        .flags = PFLAG_DRAWADD,
    );

    return 1;
}

static int marisa_star_orbit(Projectile *p, int t) { // a[0]: x' a[1]: x''
    if(t == 0) {
        p->pos0 = global.plr.pos;
    }

    if(t < 0) {
        return 1;
    }

    if(global.frames - global.plr.recovery > 0) {
        return ACTION_DESTROY;
    }

    return player_run_bomb_logic(&global.plr, p, &p->args[3], marisa_star_orbit_logic);
}

static void marisa_star_bomb(Player *plr) {
    play_sound("bomb_marisa_b");
    for(int i = 0; i < 20; i++) {
        float r = frand()*40 + 100;
        float phi = frand()*2*M_PI;

        PARTICLE("maristar_orbit", plr->pos + r*cexp(I*phi), 0, marisa_star_orbit,
            .draw_rule = marisa_star_bomb_draw,
            .args = { I*r*cexp(I*(phi+frand()*0.5))/10 },
            .type = PlrProj,
        );
    }
}

static void marisa_star_respawn_slaves(Player *plr, short npow) {
    Enemy *e = plr->slaves, *tmp;
    double dmg = 5;

    while(e != 0) {
        tmp = e;
        e = e->next;
        if(tmp->hp == ENEMY_IMMUNE)
            delete_enemy(&plr->slaves, tmp);
    }

    if(npow / 100 == 1) {
        create_enemy_p(&plr->slaves, 40.0*I, ENEMY_IMMUNE, marisa_common_slave_visual, marisa_star_slave, +30.0*I, -2.0*I, -0.1*I, dmg);
    }

    if(npow >= 200) {
        create_enemy_p(&plr->slaves, 30.0*I+15, ENEMY_IMMUNE, marisa_common_slave_visual, marisa_star_slave, +30.0*I+10, -0.3-2.0*I,  1-0.1*I, dmg);
        create_enemy_p(&plr->slaves, 30.0*I-15, ENEMY_IMMUNE, marisa_common_slave_visual, marisa_star_slave, +30.0*I-10,  0.3-2.0*I, -1-0.1*I, dmg);
    }

    if(npow / 100 == 3) {
        create_enemy_p(&plr->slaves, -30.0*I, ENEMY_IMMUNE, marisa_common_slave_visual, marisa_star_slave, +30.0*I, -2.0*I, -0.1*I, dmg);
    }

    if(npow >= 400) {
        create_enemy_p(&plr->slaves,  30, ENEMY_IMMUNE, marisa_common_slave_visual, marisa_star_slave,  25+30.0*I, -0.6-2.0*I,  2-0.1*I, dmg);
        create_enemy_p(&plr->slaves, -30, ENEMY_IMMUNE, marisa_common_slave_visual, marisa_star_slave, -25+30.0*I,  0.6-2.0*I, -2-0.1*I, dmg);
    }
}

static void marisa_star_power(Player *plr, short npow) {
    if(plr->power / 100 == npow / 100) {
        return;
    }

    marisa_star_respawn_slaves(plr, npow);
}

static void marisa_star_init(Player *plr) {
    marisa_star_respawn_slaves(plr, plr->power);
}

static void marisa_star_preload(void) {
    const int flags = RESF_DEFAULT;

    preload_resources(RES_TEXTURE, flags,
        "proj/marisa",
        "proj/maristar",
        "part/maristar_orbit",
    NULL);

    preload_resources(RES_SFX, flags | RESF_OPTIONAL,
        "bomb_marisa_b",
    NULL);
}

PlayerMode plrmode_marisa_b = {
    .name = "Star Sign",
    .character = &character_marisa,
    .shot_mode = PLR_SHOT_MARISA_STAR,
    .procs = {
        .bomb = marisa_star_bomb,
        .shot = marisa_common_shot,
        .power = marisa_star_power,
        .preload = marisa_star_preload,
        .init = marisa_star_init,
    },
};
