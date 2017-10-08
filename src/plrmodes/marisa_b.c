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

    if(p->clr) {
        parse_color_call(p->clr, glColor4f);
        draw_texture_p(0, 0, p->tex);
        glColor4f(1, 1, 1, 1);
    } else {
        draw_texture_p(0, 0, p->tex);
    }

    glPopMatrix();
}

static void marisa_star_trail_draw(Projectile *p, int t) {
    float s = 1 - t / creal(p->args[0]);

    Color clr = derive_color(p->clr, CLRMASK_A, rgba(0, 0, 0, s*0.5));

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glPushMatrix();
    glTranslatef(creal(p->pos), cimag(p->pos), 0);
    glRotatef(t*10, 0, 0, 1);
    glScalef(s, s, 1);
    parse_color_call(clr, glColor4f);
    draw_texture_p(0, 0, p->tex);
    glColor4f(1,1,1,1);
    glPopMatrix();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

static void marisa_star_bomb_draw(Projectile *p, int t) {
    marisa_star(p, t);
    create_particle1c("maristar_orbit", p->pos, 0, GrowFadeAdd, timeout, 40)->type = PlrProj;
}

static int marisa_star_projectile(Projectile *p, int t) {
    float c = 0.3 * psin(t * 0.2);
    p->clr = rgb(1 - c, 0.7 + 0.3 * psin(t * 0.1), 0.9 + c/3);

    int r = accelerated(p, t);
    create_projectile_p(&global.particles, get_tex("proj/maristar"), p->pos, p->clr, marisa_star_trail_draw, timeout, 10, 0, 0, 0)->type = PlrProj;

    if(t == EVENT_DEATH) {
        Projectile *impact = create_projectile_p(&global.particles, get_tex("proj/maristar"), p->pos, p->clr, GrowFadeAdd, timeout, 40, 2, 0, 0);
        impact->type = PlrProj;
        impact->angle = frand() * 2 * M_PI;
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

        create_projectile_p(&global.projs, get_tex("proj/maristar"), e->pos, rgb(1, 0.5, 1), marisa_star, marisa_star_projectile,
                            v, a, 0, 0)->type = PlrProj+e->args[3]*15;
    }

    e->pos = global.plr.pos + (1 - focus)*e->pos0 + focus*e->args[0];

    return 1;
}

static int marisa_star_orbit(Projectile *p, int t) { // a[0]: x' a[1]: x''
    if(t == 0)
        p->pos0 = global.plr.pos;
    if(t < 0)
        return 1;

    if(t > 300 || global.frames - global.plr.recovery > 0)
        return ACTION_DESTROY;

    float r = cabs(p->pos0 - p->pos);

    p->args[1] = (0.5e5-t*t)*cexp(I*carg(p->pos0 - p->pos))/(r*r);
    p->args[0] += p->args[1]*0.2;
    p->pos += p->args[0];

    return 1;
}

static void marisa_star_bomb(Player *plr) {
    for(int i = 0; i < 20; i++) {
        float r = frand()*40 + 100;
        float phi = frand()*2*M_PI;
        create_particle1c("maristar_orbit", plr->pos + r*cexp(I*phi), 0, marisa_star_bomb_draw, marisa_star_orbit, I*r*cexp(I*(phi+frand()*0.5))/10)->type=PlrProj;
    }
}

static void marisa_star_power(Player *plr, short npow) {
    Enemy *e = plr->slaves, *tmp;
    double dmg = 5;

    if(plr->power / 100 == npow / 100) {
        return;
    }

    while(e != 0) {
        tmp = e;
        e = e->next;
        if(tmp->hp == ENEMY_IMMUNE)
            delete_enemy(&plr->slaves, tmp);
    }

    if(npow / 100 == 1) {
        create_enemy_p(&plr->slaves, 40.0*I, ENEMY_IMMUNE, marisa_common_slave_draw, marisa_star_slave, +30.0*I, -2.0*I, -0.1*I, dmg);
    }

    if(npow >= 200) {
        create_enemy_p(&plr->slaves, 30.0*I+15, ENEMY_IMMUNE, marisa_common_slave_draw, marisa_star_slave, +30.0*I+10, -0.3-2.0*I,  1-0.1*I, dmg);
        create_enemy_p(&plr->slaves, 30.0*I-15, ENEMY_IMMUNE, marisa_common_slave_draw, marisa_star_slave, +30.0*I-10,  0.3-2.0*I, -1-0.1*I, dmg);
    }

    if(npow / 100 == 3) {
        create_enemy_p(&plr->slaves, -30.0*I, ENEMY_IMMUNE, marisa_common_slave_draw, marisa_star_slave, +30.0*I, -2.0*I, -0.1*I, dmg);
    }

    if(npow >= 400) {
        create_enemy_p(&plr->slaves,  30, ENEMY_IMMUNE, marisa_common_slave_draw, marisa_star_slave,  25+30.0*I, -0.6-2.0*I,  2-0.1*I, dmg);
        create_enemy_p(&plr->slaves, -30, ENEMY_IMMUNE, marisa_common_slave_draw, marisa_star_slave, -25+30.0*I,  0.6-2.0*I, -2-0.1*I, dmg);
    }
}

PlayerMode plrmode_marisa_b = {
    .name = "Star Sign",
    .character = &character_marisa,
    .shot_mode = PLR_SHOT_MARISA_STAR,
    .procs = {
        .bomb = marisa_star_bomb,
        .shot = marisa_common_shot,
        .power = marisa_star_power,
    },
};
