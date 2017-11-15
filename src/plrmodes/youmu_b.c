/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include <float.h>

#include "global.h"
#include "plrmodes.h"
#include "youmu.h"

static complex youmu_homing_target(complex org, complex fallback) {
    double mindst = DBL_MAX;
    complex target = fallback;

    if(global.boss) {
        target = global.boss->pos;
        mindst = cabs(target - org);
    }

    for(Enemy *e = global.enemies; e; e = e->next) {
        if(e->hp == ENEMY_IMMUNE){
            continue;
        }

        double dst = cabs(e->pos - org);

        if(dst < mindst) {
            mindst = dst;
            target = e->pos;
        }
    }

    return target;
}

static void youmu_homing_draw_common(Projectile *p, int t, float clrfactor, float alpha) {
    Color oldcolor = p->color;
    p->color = multiply_colors(oldcolor, rgba(0.7f + 0.3f * clrfactor, 0.9f + 0.1f * clrfactor, 1, alpha));
    ProjDraw(p, t);
    p->color = oldcolor;
}

static void youmu_homing_draw_proj(Projectile *p, int t) {
    float a = clamp(1.0f - (float)t / p->args[2], 0, 1);
    youmu_homing_draw_common(p, t, a, 0.5f);
}

static void youmu_homing_draw_trail(Projectile *p, int t) {
    float a = clamp(1.0f - (float)t / p->args[0], 0, 1);
    youmu_homing_draw_common(p, t, a, 0.15f * a);
}

static void youmu_homing_trail(Projectile *p, complex v, int to) {
    PARTICLE(
        .texture_ptr = p->tex,
        .pos = p->pos,
        .color = p->color,
        .angle = p->angle,
        .rule = timeout_linear,
        .draw_rule = youmu_homing_draw_trail,
        .args = { to, v },
        .type = PlrProj,
    );
}

static int youmu_homing(Projectile *p, int t) { // a[0]: velocity, a[1]: aim (r: linear, i: accelerated), a[2]: timeout, a[3]: initial target
    if(t == EVENT_DEATH) {
        return 1;
    }

    if(t > creal(p->args[2])) {
        return ACTION_DESTROY;
    }

    p->args[3] = youmu_homing_target(p->pos, p->args[3]);

    double v = cabs(p->args[0]);
    complex aimdir = cexp(I*carg(p->args[3] - p->pos));

    p->args[0] += creal(p->args[1]) * aimdir;
    p->args[0] = v * cexp(I*carg(p->args[0])) + cimag(p->args[1]) * aimdir;

    p->angle = carg(p->args[0]);
    p->pos += p->args[0];

    youmu_homing_trail(p, 0.5 * p->args[0], 12);
    return 1;
}

static int youmu_trap(Projectile *p, int t) {
    if(t == EVENT_DEATH) {
        PARTICLE("blast", p->pos, 0, blast_timeout, { 15 }, .draw_rule = Blast);
        return 1;
    }

    double expiretime = creal(p->args[1]);

    if(t > expiretime) {
        return ACTION_DESTROY;
    }

    if(!(global.plr.inputflags & INFLAG_FOCUS)) {
        PARTICLE("blast", p->pos, 0, blast_timeout, { 20 }, .draw_rule = Blast);
        PARTICLE("blast", p->pos, 0, blast_timeout, { 23 }, .draw_rule = Blast);

        int cnt = creal(p->args[2]);
        int dmg = cimag(p->args[2]);
        int dur = 45 + 10 * nfrand(); // creal(p->args[3]) + nfrand() * cimag(p->args[3]);
        complex aim = p->args[3];

        for(int i = 0; i < cnt; ++i) {
            float a = (i / (float)cnt) * M_PI * 2;
            complex dir = cexp(I*(a));

            PROJECTILE("hghost", p->pos, rgb(1, 1, 1), youmu_homing,
                .args = { 5 * dir, aim, dur, global.plr.pos },
                .type = PlrProj + dmg,
                .draw_rule = youmu_homing_draw_proj,
                .color_transform_rule = proj_clrtransform_particle,
            );
        }

        return ACTION_DESTROY;
    }

    p->angle = global.frames + t;
    p->pos += p->args[0] * (0.01 + 0.99 * max(0, (10 - t) / 10.0));

    youmu_homing_trail(p, cexp(I*p->angle), 30);
    return 1;
}

static void YoumuSlash(Enemy *e, int t, bool render) {
    if(render) {
        t = player_get_bomb_progress(&global.plr, NULL);
        fade_out(10.0/t+sin(t/10.0)*0.1);
    }
}

static int youmu_slash_logic(void *v, int t, double speed) {
    Enemy *e = v;
    TIMER(&t);

    AT(0)
        global.plr.pos = VIEWPORT_W/5.0 + (VIEWPORT_H - 100)*I;

    FROM_TO(8,20,1)
        global.plr.pos = VIEWPORT_W + (VIEWPORT_H - 100)*I - exp(-_i/8.0+log(4*VIEWPORT_W/5.0));

    FROM_TO(30, 60, 10) {
        tsrand_fill(3);

        PARTICLE(
            .texture = "youmu_slice",
            .pos = VIEWPORT_W/2.0 - 150 + 100*_i + VIEWPORT_H/2.0*I - 10-10.0*I + 20*afrand(0)+20.0*I*afrand(1),
            .draw_rule = youmu_common_particle_slice_draw,
            .rule = youmu_common_particle_slice_logic,
            .args = { 200 / speed },
            .angle = 10 * anfrand(2),
            .type = PlrProj,
        );
    }

    FROM_TO(40,200,1)
        if(frand() > 0.7) {
            tsrand_fill(6);
            PARTICLE(
                .texture = "blast",
                .pos = VIEWPORT_W*afrand(0) + (VIEWPORT_H+50)*I,
                .color = rgb(afrand(1),afrand(2),afrand(3)),
                .rule = timeout_linear,
                .draw_rule = Shrink,
                .args = { 80 / speed, speed * (3*(1-2.0*afrand(4))-14.0*I+afrand(5)*2.0*I) },
                .type = PlrProj,
            );
        }

    int tpar = 30;
    if(t < 30)
        tpar = t;

    if(t < creal(e->args[0])-60 && frand() > 0.2) {
        tsrand_fill(3);
        PARTICLE(
            .texture = "smoke",
            .pos = VIEWPORT_W*afrand(0) + (VIEWPORT_H+100)*I,
            .color = rgba(0.4,0.4,0.4,afrand(1)*0.2 - 0.2 + 0.6*(tpar/30.0)),
            .rule = youmu_common_particle_spin,
            .args = { 300 / speed, speed * (-7.0*I+afrand(2)*1.0*I) },
            .type = PlrProj,
        );
    }

    return 1;
}

static int youmu_slash(Enemy *e, int t) {
    if(t > creal(e->args[0]))
        return ACTION_DESTROY;
    if(t < 0)
        return 1;

    if(global.frames - global.plr.recovery > 0) {
        return ACTION_DESTROY;
    }

    return player_run_bomb_logic(&global.plr, e, &e->args[3], youmu_slash_logic);
}

static void youmu_haunting_power_shot(Player *plr, int p) {
    int d = -2;
    double spread = 4;
    complex aim = (0.5 + 0.1 * p) + (0.1 - p * 0.025) * I;
    double speed = 10;

    if(plr->power / 100 < p || (global.frames + d * p) % 12) {
        return;
    }

    Texture *t = get_tex("proj/hghost");

    for(int sign = -1; sign < 2; sign += 2) {
        PROJECTILE(
            .texture_ptr = t,
            .pos =  plr->pos,
            .rule = youmu_homing,
            .draw_rule = youmu_homing_draw_proj,
            .args = { speed * cexp(I*carg(sign*p*spread-speed*I)), aim, 60, VIEWPORT_W*0.5 },
            .type = PlrProj+54,
            .color_transform_rule = proj_clrtransform_particle,
        );
    }
}

static void youmu_haunting_shot(Player *plr) {
    youmu_common_shot(plr);

    if(player_should_shoot(plr, true)) {
        if(plr->inputflags & INFLAG_FOCUS) {
            int pwr = plr->power / 100;

            if(!(global.frames % (45 - 4 * pwr))) {
                int pcnt = 11 + pwr * 4;
                int pdmg = 120 - 18 * 4 * (1 - pow(1 - pwr / 4.0, 1.5));
                complex aim = 0.75;

                PROJECTILE("youhoming", plr->pos, rgb(1, 1, 1), youmu_trap,
                    .args = { -30.0*I, 120, pcnt+pdmg*I, aim },
                    .type = PlrProj+1000,
                    .color_transform_rule = proj_clrtransform_particle,
                );
            }
        } else {
            if(!(global.frames % 6)) {
                PROJECTILE("hghost", plr->pos, rgb(0.75, 0.9, 1), youmu_homing,
                    .args = { -10.0*I, 0.25 + 0.1*I, 60, VIEWPORT_W*0.5 },
                    .type = PlrProj+120,
                    .color_transform_rule = proj_clrtransform_particle,
                );
            }

            for(int p = 1; p <= PLR_MAX_POWER/100; ++p) {
                youmu_haunting_power_shot(plr, p);
            }
        }
    }
}

static void youmu_haunting_bomb(Player *plr) {
    play_sound("bomb_youmu_b");
    create_enemy_p(&plr->slaves, 40.0*I, ENEMY_BOMB, YoumuSlash, youmu_slash, 280,0,0,0);
}

static void youmu_haunting_preload(void) {
    const int flags = RESF_DEFAULT;

    preload_resources(RES_TEXTURE, flags,
        "proj/youmu",
        "part/youmu_slice",

    NULL);

    preload_resources(RES_SFX, flags | RESF_OPTIONAL,
        "bomb_youmu_b",
    NULL);
}

PlayerMode plrmode_youmu_b = {
    .name = "Haunting Sign",
    .character = &character_youmu,
    .shot_mode = PLR_SHOT_YOUMU_HAUNTING,
    .procs = {
        .bomb = youmu_haunting_bomb,
        .shot = youmu_haunting_shot,
        .preload = youmu_haunting_preload,
    },
};
