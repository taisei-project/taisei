/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "global.h"
#include "plrmodes.h"
#include "youmu.h"

static int myon_particle_rule(Projectile *p, int t) {
    float a = 1.0;

    if(t > 0) {
        double mt = creal(p->args[0]);
        a *= (mt - t) / mt;
    }

    p->clr = mix_colors(
        rgba(0.85, 0.9, 1.0, 1.0),
        rgba(0.5, 0.7, 1.0, a),
    1 - pow(1 - a, 1 + psin((t + global.frames) * 0.1)));

    return timeout_linear(p, t);
}

static void myon_particle_draw(Projectile *p, int t) {
    // hack to bypass the bullet color shader
    Color clr = p->clr;
    p->clr = 0;
    parse_color_call(clr, glColor4f);
    Shrink(p, t);
    glColor4f(1, 1, 1, 1);
    p->clr = clr;
}

static void myon_draw(Enemy *e, int t) {
    float a = global.frames * 0.07;
    complex pos = e->pos + 3 * (cos(a) + I * sin(a));

    complex dir = cexp(I*(0.1 * sin(global.frames * 0.05) + carg(global.plr.pos - e->pos)));
    double v = 4 * cabs(global.plr.pos - e->pos) / (VIEWPORT_W * 0.5);

    create_particle2c("flare", pos, 0, myon_particle_draw, myon_particle_rule, 20, v*dir)->type = PlrProj;
}

static void youmu_mirror_myon_proj(char *tex, complex pos, double speed, double angle, double aoffs, double upfactor, int dmg) {
    complex dir = cexp(I*(M_PI/2 + aoffs)) * upfactor + cexp(I * (angle + aoffs)) * (1 - upfactor);
    dir = dir / cabs(dir);
    create_projectile1c(tex, pos, 0, linear, speed*dir)->type = PlrProj+dmg;
}

static int youmu_mirror_myon(Enemy *e, int t) {
    if(t == EVENT_BIRTH)
        e->pos = e->pos0 + global.plr.pos;
    if(t < 0)
        return 1;

    Player *plr = &global.plr;
    float rad = cabs(e->pos0);

    double nfocus = plr->focus / 30.0;

    if(!(plr->inputflags & INFLAG_FOCUS)) {
        if((plr->inputflags & INFLAGS_MOVE)) {
            e->pos0 = rad * -plr->lastmovedir;
        } else {
            e->pos0 = e->pos - plr->pos;
            e->pos0 *= rad / cabs(e->pos0);
        }
    }

    complex target = plr->pos + e->pos0;
    e->pos += cexp(I*carg(target - e->pos)) * min(10, 0.07 * max(0, cabs(target - e->pos) - VIEWPORT_W * 0.5 * nfocus));

    if(!(plr->inputflags & INFLAG_FOCUS)) {
        e->args[0] = plr->pos - e->pos;
    }

    if(player_should_shoot(&global.plr, true) && !(global.frames % 6) && global.plr.deathtime >= -1) {
        int v1 = -21;
        int v2 = -10;

        double r1 = (psin(global.frames * 2) * 0.5 + 0.5) * 0.1;
        double r2 = (psin(global.frames * 1.2) * 0.5 + 0.5) * 0.1;

        double a = carg(e->args[0]);
        double u = 1 - nfocus;

        int p = plr->power / 100;
        int dmg_center = 160 - 30 * p;
        int dmg_side = 23 + 2 * p;

        if(plr->power >= 100) {
            youmu_mirror_myon_proj("youmu",  e->pos, v1, a,  r1*1, u, dmg_side);
            youmu_mirror_myon_proj("youmu",  e->pos, v1, a, -r1*1, u, dmg_side);
        }

        if(plr->power >= 200) {
            youmu_mirror_myon_proj("hghost", e->pos, v2, a,  r2*2, 0, dmg_side);
            youmu_mirror_myon_proj("hghost", e->pos, v2, a, -r2*2, 0, dmg_side);
        }

        if(plr->power >= 300) {
            youmu_mirror_myon_proj("youmu",  e->pos, v1, a,  r1*3, u, dmg_side);
            youmu_mirror_myon_proj("youmu",  e->pos, v1, a, -r1*3, u, dmg_side);
        }

        if(plr->power >= 400) {
            youmu_mirror_myon_proj("hghost", e->pos, v2, a,  r2*4, 0, dmg_side);
            youmu_mirror_myon_proj("hghost", e->pos, v2, a, -r2*4, 0, dmg_side);
        }

        youmu_mirror_myon_proj("hghost", e->pos, v2, a, 0, 0, dmg_center);
    }

    return 1;
}

static int youmu_split(Enemy *e, int t) {
    if(t < 0)
        return 1;

    if(t > creal(e->args[0]))
        return ACTION_DESTROY;

    if(global.frames - global.plr.recovery > 0) {
        return ACTION_DESTROY;
    }

    TIMER(&t);

    FROM_TO(30,200,1) {
        tsrand_fill(2);
        create_particle2c("smoke", VIEWPORT_W/2 + VIEWPORT_H/2*I, rgba(0.4,0.4,0.4,afrand(0)*0.2+0.4), PartDraw, youmu_common_particle_spin, 300, 6*cexp(I*afrand(1)*2*M_PI));
    }

    FROM_TO(100,170,10) {
        tsrand_fill(3);
        create_particle1c("youmu_slice", VIEWPORT_W/2.0 + VIEWPORT_H/2.0*I - 200-200.0*I + 400*afrand(0)+400.0*I*afrand(1), 0, youmu_common_particle_slice_draw, timeout, 100-_i)->angle = 360.0*afrand(2);
    }


    FROM_TO(0, 220, 1) {
        float talt = atan((t-e->args[0]/2)/30.0)*10+atan(-e->args[0]/2);
        global.plr.pos = VIEWPORT_W/2.0 + (VIEWPORT_H-80)*I + VIEWPORT_W/3.0*sin(talt);
    }

    return 1;
}

static void youmu_mirror_bomb(Player *plr) {
    create_enemy_p(&plr->slaves, 40.0*I, ENEMY_BOMB, NULL, youmu_split, 280,0,0,0);
}

static void youmu_mirror_think(Player *plr) {
    // XXX: do we really have to do this every frame?

    if(plr->slaves == NULL) {
        create_enemy_p(&plr->slaves, 40.0*I, ENEMY_IMMUNE, myon_draw, youmu_mirror_myon, 0, 0, 0, 0);
    }
}

PlayerMode plrmode_youmu_a = {
    .name = "Mirror Sign",
    .character = &character_youmu,
    .shot_mode = PLR_SHOT_YOUMU_MIRROR,
    .procs = {
        .bomb = youmu_mirror_bomb,
        .shot = youmu_common_shot,
        .think = youmu_mirror_think,
    },
};
