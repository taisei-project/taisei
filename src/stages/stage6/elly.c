/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "elly.h"
#include "stage6.h"
#include "extra.h"
#include "draw.h"

#include "common_tasks.h"

void scythe_common(Enemy *e, int t) {
    e->args[1] += cimag(e->args[1]);
}

int wait_proj(Projectile *p, int t) {
    if(t < 0) {
        return ACTION_ACK;
    }

    if(t > creal(p->args[1])) {
        if(t == creal(p->args[1]) + 1) {
            play_sfx_ex("redirect", 4, false);
            spawn_projectile_highlight_effect(p);
        }

        p->angle = carg(p->args[0]);
        p->pos += p->args[0];
    }

    return ACTION_NONE;
}

int scythe_mid(Enemy *e, int t) {
    TIMER(&t);
    cmplx n;

    if(t < 0) {
        scythe_common(e, t);
        return 1;
    }

    if(creal(e->pos) > VIEWPORT_W + 100) {
        return ACTION_DESTROY;
    }

    e->pos += (6-global.diff-0.005*I*t)*e->args[0];

    n = cexp(cimag(e->args[1])*I*t);
    FROM_TO_SND("shot1_loop",0,300,1) {}
    PROJECTILE(
        .proto = pp_bigball,
        .pos = e->pos + 80*n,
        .color = RGBA(0.2, 0.5-0.5*cimag(n), 0.5+0.5*creal(n), 0.0),
        .rule = wait_proj,
        .args = {
            global.diff*cexp(0.6*I)*n,
            100
        },
    );

    if(global.diff > D_Normal && t&1) {
        Projectile *p = PROJECTILE(
            .proto = pp_ball,
            .pos = e->pos + 80*n,
            .color = RGBA(0, 0.2, 0.5, 0.0),
            .rule = accelerated,
            .args = {
                n,
                0.01*global.diff*cexp(I*carg(global.plr.pos - e->pos - 80*n))
            },
        );

        if(projectile_in_viewport(p)) {
            play_sfx("shot1");
        }
    }

    scythe_common(e, t);
    return 1;
}

DEPRECATED_DRAW_RULE
static void scythe_draw_trail(Projectile *p, int t, ProjDrawRuleArgs args) {
    r_mat_mv_push();
    r_mat_mv_translate(creal(p->pos), cimag(p->pos), 0);
    r_mat_mv_rotate(p->angle + (M_PI * 0.5), 0, 0, 1);
    r_mat_mv_scale(creal(p->args[1]), creal(p->args[1]), 1);
    float a = (1.0 - t/p->timeout) * (1.0 - cimag(p->args[1]));
    ProjDrawCore(p, RGBA(a, a, a, a));
    r_mat_mv_pop();
}

void scythe_draw(Enemy *e, int t, bool render) {
    if(render) {
        return;
    }

    PARTICLE(
        .sprite_ptr = get_sprite("stage6/scythe"),
        .pos = e->pos+I*6*sin(global.frames/25.0),
        .draw_rule = scythe_draw_trail,
        .timeout = 8,
        .args = { 0, e->args[2] },
        .angle = creal(e->args[1]) - M_PI/2,
        .flags = PFLAG_REQUIREDPARTICLE,
    );

    PARTICLE(
        .sprite = "smoothdot",
        .pos = e->pos+100*creal(e->args[2])*frand()*cexp(2.0*I*M_PI*frand()),
        .color = RGBA(1.0, 0.1, 1.0, 0.0),
        .draw_rule = GrowFade,
        .rule = linear,
        .timeout = 60,
        .args = { -I+1, -I+1 }, // XXX: what the fuck?
        .flags = PFLAG_REQUIREDPARTICLE,
    );
}

int scythe_infinity(Enemy *e, int t) {
    if(t < 0) {
        scythe_common(e, t);
        return 1;
    }

    TIMER(&t);
    FROM_TO(0, 40, 1) {
        GO_TO(e, VIEWPORT_W/2+200.0*I, 0.01);
        e->args[2] = fmin(0.8, creal(e->args[2])+0.0003*t*t);
        e->args[1] = creal(e->args[1]) + I*fmin(0.2, cimag(e->args[1])+0.0001*t*t);
    }

    FROM_TO_SND("shot1_loop",40, 3000, 1) {
        float w = fmin(0.15, 0.0001*(t-40));
        e->pos = VIEWPORT_W/2 + 200.0*I + 200*cos(w*(t-40)+M_PI/2.0) + I*80*sin(creal(e->args[0])*w*(t-40));

        PROJECTILE(
            .proto = pp_ball,
            .pos = e->pos+80*cexp(I*creal(e->args[1])),
            .color = RGB(cos(creal(e->args[1])), sin(creal(e->args[1])), cos(creal(e->args[1])+2.1)),
            .rule = asymptotic,
            .args = {
                (1+0.4*global.diff)*cexp(I*creal(e->args[1])),
                3 + 0.2 * global.diff
            }
        );
    }

    scythe_common(e, t);
    return 1;
}

int scythe_reset(Enemy *e, int t) {
    if(t < 0) {
        scythe_common(e, t);
        return 1;
    }

    if(t == 1)
        e->args[1] = fmod(creal(e->args[1]), 2*M_PI) + I*cimag(e->args[1]);

    GO_TO(e, BOSS_DEFAULT_GO_POS, 0.05);
    e->args[2] = fmax(0.6, creal(e->args[2])-0.01*t);
    e->args[1] += (0.19-creal(e->args[1]))*0.05;
    e->args[1] = creal(e->args[1]) + I*0.9*cimag(e->args[1]);

    scythe_common(e, t);
    return 1;
}

attr_returns_nonnull
Enemy* find_scythe(void) {
    for(Enemy *e = global.enemies.first; e; e = e->next) {
        if(e->visual_rule == scythe_draw) {
            return e;
        }
    }

    UNREACHABLE;
}

void elly_clap(Boss *b, int claptime) {
    aniplayer_queue(&b->ani, "clapyohands", 1);
    aniplayer_queue_frames(&b->ani, "holdyohands", claptime);
    aniplayer_queue(&b->ani, "unclapyohands", 1);
    aniplayer_queue(&b->ani, "main", 0);
}

static int baryon_unfold(Enemy *baryon, int t) {
    if(t < 0)
        return 1; // not catching death for references! because there will be no death!

    TIMER(&t);

    int extent = 100;
    float timeout;

    if(global.stage->type == STAGE_SPELL) {
        timeout = 92;
    } else {
        timeout = 142;
    }

    FROM_TO(0, timeout, 1) {
        for(Enemy *e = global.enemies.first; e; e = e->next) {
            if(e->visual_rule == baryon_center_draw) {
                float f = t / (float)timeout;
                float x = f;
                float g = sin(2 * M_PI * log(log(x + 1) + 1));
                float a = g * pow(1 - x, 2 * x);
                f = 1 - pow(1 - f, 3) + a;

                baryon->pos = baryon->pos0 = e->pos + baryon->args[0] * f * extent;
                return 1;
            }
        }
    }

    return 1;
}

static int elly_baryon_center(Enemy *e, int t) {
    if(t == EVENT_DEATH) {
        free_ref(creal(e->args[1]));
        free_ref(cimag(e->args[1]));
    }

    if(global.boss) {
        e->pos = global.boss->pos;
    }

    return 1;
}

int scythe_explode(Enemy *e, int t) {
    if(t < 0) {
        scythe_common(e, t);
        return 0;
    }

    if(t < 50) {
        e->args[1] += 0.001*I*t;
        e->args[2] += 0.0015*t;
    }

    if(t >= 50)
        e->args[2] -= 0.002*(t-50);

    if(t == 100) {
        petal_explosion(100, e->pos);
        stage_shake_view(16);
        play_sfx("boom");

        scythe_common(e, t);
        return ACTION_DESTROY;
    }

    scythe_common(e, t);
    return 1;
}

void elly_spawn_baryons(cmplx pos) {
    int i;
    Enemy *e, *last = NULL, *first = NULL, *middle = NULL;

    for(i = 0; i < 6; i++) {
        e = create_enemy3c(pos, ENEMY_IMMUNE, baryon_center_draw, baryon_unfold, 1.5*cexp(2.0*I*M_PI/6*i), i != 0 ? add_ref(last) : 0, i);
        e->ent.draw_layer = LAYER_BACKGROUND;

        if(i == 0) {
            first = e;
        } else if(i == 3) {
            middle = e;
        }

        last = e;
    }

    first->args[1] = add_ref(last);
    e = create_enemy2c(pos, ENEMY_IMMUNE, baryon_center_draw, elly_baryon_center, 0, add_ref(first) + I*add_ref(middle));
    e->ent.draw_layer = LAYER_BACKGROUND;
}


void set_baryon_rule(EnemyLogicRule r) {
    Enemy *e;
    for(e = global.enemies.first; e; e = e->next) {
        if(e->visual_rule == baryon) {
            e->birthtime = global.frames;
            e->logic_rule = r;
        }
    }
}

int baryon_reset(Enemy *baryon, int t) {
    if(t < 0) {
        return 1;
    }

    for(Enemy *e = global.enemies.first; e; e = e->next) {
        if(e->visual_rule == baryon_center_draw) {
            cmplx targ_pos = baryon->pos0 - e->pos0 + e->pos;
            GO_TO(baryon, targ_pos, 0.1);

            return 1;
        }
    }

    GO_TO(baryon, baryon->pos0, 0.1);
    return 1;
}

static int broglie_particle(Projectile *p, int t) {
    if(t == EVENT_DEATH) {
        free_ref(p->args[0]);
        return ACTION_ACK;
    }

    if(t < 0) {
        return ACTION_ACK;
    }

    int scattertime = creal(p->args[1]);

    if(t < scattertime) {
        Laser *laser = (Laser*)REF(p->args[0]);

        if(laser) {
            cmplx oldpos = p->pos;
            p->pos = laser->prule(laser, fmin(t, cimag(p->args[1])));

            if(oldpos != p->pos) {
                p->angle = carg(p->pos - oldpos);
            }
        }
    } else {
        if(t == scattertime && p->type != PROJ_DEAD) {
            projectile_set_layer(p, LAYER_BULLET);
            p->flags &= ~(PFLAG_NOCLEARBONUS | PFLAG_NOCLEAREFFECT | PFLAG_NOCOLLISION);

            double angle_ampl = creal(p->args[3]);
            double angle_freq = cimag(p->args[3]);

            p->angle += angle_ampl * sin(t * angle_freq) * cos(2 * t * angle_freq);
            p->args[2] = -cabs(p->args[2]) * cexp(I*p->angle);

            play_sfx("redirect");
        }

        p->angle = carg(p->args[2]);
        p->pos = p->pos + p->args[2];
    }

    return 1;
}

static void broglie_laser_logic(Laser *l, int t) {
    double hue = cimag(l->args[3]);

    if(t == EVENT_BIRTH) {
        l->color = *HSLA(hue, 1.0, 0.5, 0.0);
    }

    if(t < 0) {
        return;
    }

    int dt = l->timespan * l->speed;
    float charge = fmin(1, pow((double)t / dt, 4));
    l->color = *HSLA(hue, 1.0, 0.5 + 0.2 * charge, 0.0);
    l->width_exponent = 1.0 - 0.5 * charge;
}

int broglie_charge(Projectile *p, int t) {
    if(t == EVENT_BIRTH) {
        play_sfx_ex("shot3", 10, false);
    }

    if(t < 0) {
        return ACTION_ACK;
    }

    int firetime = creal(p->args[1]);

    if(t == firetime) {
        play_sfx_ex("laser1", 10, true);
        play_sfx("boom");

        stage_shake_view(20);

        Color clr = p->color;
        clr.a = 0;

        PARTICLE(
            .sprite = "blast",
            .pos = p->pos,
            .color = &clr,
            .timeout = 35,
            .draw_rule = GrowFade,
            .args = { 0, 2.4 },
            .flags = PFLAG_REQUIREDPARTICLE,
        );

        int attack_num = creal(p->args[2]);
        double hue = creal(p->args[3]);

        p->pos -= p->args[0] * 15;
        cmplx aim = cexp(I*p->angle);

        double s_ampl = 30 + 2 * attack_num;
        double s_freq = 0.10 + 0.01 * attack_num;

        for(int lnum = 0; lnum < 2; ++lnum) {
            Laser *l = create_lasercurve4c(p->pos, 75, 100, RGBA(1, 1, 1, 0), las_sine,
                5*aim, s_ampl, s_freq, lnum * M_PI +
                I*(hue + lnum * (M_PI/12)/(M_PI/2)));

            l->width = 20;
            l->lrule = broglie_laser_logic;

            int pnum = 0;
            double inc = pow(1.10 - 0.10 * (global.diff - D_Easy), 2);

            for(double ofs = 0; ofs < l->deathtime; ofs += inc, ++pnum) {
                bool fast = global.diff == D_Easy || pnum & 1;

                PROJECTILE(
                    .proto = fast ? pp_thickrice : pp_rice,
                    .pos = l->prule(l, ofs),
                    .color = HSLA(hue + lnum * (M_PI/12)/(M_PI/2), 1.0, 0.5, 0.0),
                    .rule = broglie_particle,
                    .args = {
                        add_ref(l),
                        I*ofs + l->timespan + ofs - 10,
                        fast ? 2.0 : 1.5,
                        (1 + 2 * ((global.diff - 1) / (double)(D_Lunatic - 1))) * M_PI/11 + s_freq*10*I
                    },
                    .layer = LAYER_NODRAW,
					.flags = PFLAG_NOCLEARBONUS | PFLAG_NOCLEAREFFECT | PFLAG_NOSPAWNEFFECTS | PFLAG_NOCOLLISION,
                );
            }
        }

        return ACTION_DESTROY;
    } else {
        float f = pow(clamp((140 - (firetime - t)) / 90.0, 0, 1), 8);
        cmplx o = p->pos - p->args[0] * 15;
        p->args[0] *= cexp(I*M_PI*0.2*f);
        p->pos = o + p->args[0] * 15;

        if(f > 0.1) {
            play_sfx_loop("charge_generic");

            cmplx n = cexp(2.0*I*M_PI*frand());
            float l = 50*frand()+25;
            float s = 4+f;

            Color *clr = color_lerp(RGB(1, 1, 1), &p->color, clamp((1 - f * 0.5), 0.0, 1.0));
            clr->a = 0;

            PARTICLE(
                .sprite = "flare",
                .pos = p->pos+l*n,
                .color = clr,
                .draw_rule = Fade,
				.rule = linear,
                .timeout = l/s,
                .args = { -s*n },
            );
        }
    }

    return ACTION_NONE;
}

int baryon_explode(Enemy *e, int t) {
    TIMER(&t);
    AT(EVENT_DEATH) {
        free_ref(e->args[1]);
        petal_explosion(24, e->pos);
        play_sfx("boom");
        stage_shake_view(15);

        for(uint i = 0; i < 3; ++i) {
            PARTICLE(
                .sprite = "smoke",
                .pos = e->pos+10*frand()*cexp(2.0*I*M_PI*frand()),
                .color = RGBA(0.2 * frand(), 0.5, 0.4 * frand(), 1.0),
                .rule = asymptotic,
                .draw_rule = ScaleFade,
                .args = { cexp(I*2*M_PI*frand()) * 0.25, 2, (1 + 3*I) },
                .timeout = 600 + 50 * nfrand(),
                .layer = LAYER_PARTICLE_HIGH | 0x40,
            );
        }

        PARTICLE(
            .sprite = "blast_huge_halo",
                        .pos = e->pos + cexp(4*frand() + I*M_PI*frand()*2),
            .color = RGBA(0.2 + frand() * 0.1, 0.6 + 0.4 * frand(), 0.5 + 0.5 * frand(), 0),
            .timeout = 500 + 24 * frand() + 5,
            .draw_rule = ScaleFade,
            .args = { 0, 0, (0.25 + 2.5*I) * (1 + 0.2 * frand()) },
            .layer = LAYER_PARTICLE_HIGH | 0x41,
            .angle = frand() * 2 * M_PI,
            .flags = PFLAG_REQUIREDPARTICLE,
        );

        PARTICLE(
            .sprite = "blast_huge_rays",
            .color = color_add(RGBA(0.2 + frand() * 0.3, 0.5 + 0.5 * frand(), frand(), 0.0), RGBA(1, 1, 1, 0)),
            .pos = e->pos,
            .timeout = 300 + 38 * frand(),
            .draw_rule = ScaleFade,
            .args = { 0, 0, (0 + 5*I) * (1 + 0.2 * frand()) },
            .angle = frand() * 2 * M_PI,
            .layer = LAYER_PARTICLE_HIGH | 0x42,
            .flags = PFLAG_REQUIREDPARTICLE,
        );

        PARTICLE(
            .sprite = "blast_huge_halo",
            .pos = e->pos,
            .color = RGBA(3.0, 1.0, 2.0, 1.0),
            .timeout = 60 + 5 * nfrand(),
            .draw_rule = ScaleFade,
            .args = { 0, 0, (0.25 + 3.5*I) * (1 + 0.2 * frand()) },
            .layer = LAYER_PARTICLE_HIGH | 0x41,
            .angle = frand() * 2 * M_PI,
        );

        return 1;
    }

    GO_TO(e, global.boss->pos + (e->pos0-global.boss->pos)*(1.0+0.2*sin(t*0.1)) * cexp(I*t*t*0.0004), 0.05);

    if(!(t % 6)) {
        PARTICLE(
            .sprite = "blast_huge_halo",
            .pos = e->pos,
            .color = RGBA(3.0, 1.0, 2.0, 1.0),
            .timeout = 10,
            .draw_rule = ScaleFade,
            .args = { 0, 0, (t / 120.0 + 0*I) * (1 + 0.2 * frand()) },
            .layer = LAYER_PARTICLE_HIGH | 0x41,
            .angle = frand() * 2 * M_PI,
            .flags = PFLAG_REQUIREDPARTICLE,
        );
    }/* else {
        PARTICLE(
            .sprite = "smoke",
            .pos = e->pos+10*frand()*cexp(2.0*I*M_PI*frand()),
            .color = RGBA(0.2 * frand(), 0.5, 0.4 * frand(), 0.1 * frand()),
            .rule = asymptotic,
            .draw_rule = ScaleFade,
            .args = { cexp(I*2*M_PI*frand()) * (1 + t/300.0), (1 + t / 200.0), (1 + 0.5*I) * (t / 200.0) },
            .timeout = 60,
            .layer = LAYER_PARTICLE_HIGH | 0x40,
        );
    }*/

    if(t > 120 && frand() < (t - 120) / 300.0) {
        e->hp = 0;
        return 1;
    }

    return 1;
}

int baryon_nattack(Enemy *e, int t) {
    if(t < 0)
        return 1;

    TIMER(&t);

    e->pos = global.boss->pos + (e->pos-global.boss->pos)*cexp(0.006*I);

    FROM_TO_SND("shot1_loop",30, 10000, (7 - global.diff)) {
        float a = 0.2*_i + creal(e->args[2]) + 0.006*t;
        float ca = a + t/60.0f;
        PROJECTILE(
            .proto = pp_ball,
            .pos = e->pos+40*cexp(I*a),
            .color = RGB(cos(ca), sin(ca), cos(ca+2.1)),
            .rule = asymptotic,
            .args = {
                (1+0.2*global.diff)*cexp(I*a),
                3
            }
        );
    }

    return 1;
}


static int elly_toe_boson_effect(Projectile *p, int t) {
    if(t < 0) {
        return ACTION_ACK;
    }

    p->angle = creal(p->args[3]) * fmax(0, t) / (double)p->timeout;
    return ACTION_NONE;
}

Color* boson_color(Color *out_clr, int pos, int warps) {
    float f = pos / 3.0;
    *out_clr = *HSL((warps-0.3*(1-f))/3.0, 1+f, 0.5);
    return out_clr;
}

int elly_toe_boson(Projectile *p, int t) {
    if(t < 0) {
        return ACTION_ACK;
    }

    p->angle = carg(p->args[0]);

    int activate_time = creal(p->args[2]);
    int num_in_trail = cimag(p->args[2]);
    assert(activate_time >= 0);

    Color thiscolor_additive = p->color;
    thiscolor_additive.a = 0;

    if(t < activate_time) {
        float fract = clamp(2 * (t+1) / (float)activate_time, 0, 1);

        float a = 0.1;
        float x = fract;
        float modulate = sin(x * M_PI) * (0.5 + cos(x * M_PI));
        fract = (pow(a, x) - 1) / (a - 1);
        fract *= (1 + (1 - x) * modulate);

        p->pos = p->args[3] * fract + p->pos0 * (1 - fract);

        if(t % 3 == 0) {
            PARTICLE(
                .sprite = "smoothdot",
                .pos = p->pos,
                .color = &thiscolor_additive,
                .rule = linear,
                .timeout = 10,
                .draw_rule = Fade,
                .args = { 0, p->args[0] },
            );
        }

        return ACTION_NONE;
    }

    if(t == activate_time && num_in_trail == 2) {
        play_sfx("shot_special1");
        play_sfx("redirect");

            PARTICLE(
                .sprite = "blast",
                .pos = p->pos,
                .color = HSLA(carg(p->args[0]),0.5,0.5,0),
                .rule = elly_toe_boson_effect,
                .draw_rule = ScaleFade,
                .timeout = 60,
                .args = {
                    0,
                    p->args[0] * 1.0,
                    0. + 1.0 * I,
                    M_PI*2*frand(),
                },
                .angle = 0,
                .flags = PFLAG_REQUIREDPARTICLE,
            );
    }

    p->pos += p->args[0];
    cmplx prev_pos = p->pos;
    int warps_left = creal(p->args[1]);
    int warps_initial = cimag(p->args[1]);

    if(wrap_around(&p->pos) != 0) {
        p->prevpos = p->pos; // don't lerp

        if(warps_left-- < 1) {
            return ACTION_DESTROY;
        }

        p->args[1] = warps_left + warps_initial * I;

        if(num_in_trail == 3) {
            play_sfx_ex("warp", 0, false);
            // play_sound("redirect");
        }

        PARTICLE(
            .sprite = "myon",
            .pos = prev_pos,
            .color = &thiscolor_additive,
            .timeout = 50,
            .draw_rule = ScaleFade,
            .args = { 0, 0, 5.00 },
            .angle = M_PI*2*frand(),
        );

        PARTICLE(
            .sprite = "stardust",
            .pos = prev_pos,
            .color = &thiscolor_additive,
            .timeout = 60,
            .draw_rule = ScaleFade,
            .args = { 0, 0, 2 * I },
            .angle = M_PI*2*frand(),
            .flags = PFLAG_REQUIREDPARTICLE,
        );

        boson_color(&p->color, num_in_trail, warps_initial - warps_left);
        thiscolor_additive = p->color;
        thiscolor_additive.a = 0;

        PARTICLE(
            .sprite = "stardust",
            .pos = p->pos,
            .color = &thiscolor_additive,
            .rule = elly_toe_boson_effect,
            .draw_rule = ScaleFade,
            .timeout = 30,
            .args = { 0, p->args[0] * 2, 2 * I, M_PI*2*frand() },
            .angle = 0,
            .flags = PFLAG_REQUIREDPARTICLE,
        );
    }

    float tLookahead = 40;
    cmplx posLookahead = p->pos+p->args[0]*tLookahead;
    cmplx dir = wrap_around(&posLookahead);
    if(dir != 0 && t%3 == 0 && warps_left > 0) {
        cmplx pos0 = posLookahead - VIEWPORT_W/2*(1-creal(dir))-I*VIEWPORT_H/2*(1-cimag(dir));

        // Re [a b^*] behaves like the 2D vector scalar product
        float tOvershoot = creal(pos0*conj(dir))/creal(p->args[0]*conj(dir));
        posLookahead -= p->args[0]*tOvershoot;

        Color *clr = boson_color(&(Color){0}, num_in_trail, warps_initial - warps_left + 1);
        clr->a = 0;

        PARTICLE(
            .sprite = "smoothdot",
            .pos = posLookahead,
            .color = clr,
            .timeout = 10,
            .rule = linear,
            .draw_rule = Fade,
            .args = { p->args[0] },
            .flags = PFLAG_REQUIREDPARTICLE,
        );
    }

    return ACTION_NONE;
}

static cmplx elly_toe_laser_pos(Laser *l, float t) { // a[0]: direction, a[1]: type, a[2]: width
    int type = creal(l->args[1]+0.5);

    if(t == EVENT_BIRTH) {
        switch(type) {
        case 0:
            l->shader = r_shader_get_optional("lasers/elly_toe_fermion");
            l->color = *RGBA(0.4, 0.4, 1.0, 0.0);
            break;
        case 1:
            l->shader = r_shader_get_optional("lasers/elly_toe_photon");
            l->color = *RGBA(1.0, 0.4, 0.4, 0.0);
            break;
        case 2:
            l->shader = r_shader_get_optional("lasers/elly_toe_gluon");
            l->color = *RGBA(0.4, 1.0, 0.4, 0.0);
            break;
        case 3:
            l->shader = r_shader_get_optional("lasers/elly_toe_higgs");
            l->color = *RGBA(1.0, 0.4, 1.0, 0.0);
            break;
        default:
            log_fatal("Unknown Elly laser type.");
        }
        return 0;
    }

    double width = creal(l->args[2]);
    switch(type) {
    case 0:
        return l->pos+l->args[0]*t;
    case 1:
        return l->pos+l->args[0]*(t+width*I*sin(t/width));
    case 2:
        return l->pos+l->args[0]*(t+width*(0.6*(cos(3*t/width)-1)+I*sin(3*t/width)));
    case 3:
        return l->pos+l->args[0]*(t+floor(t/width)*width);
    }

    return 0;
}

static int elly_toe_laser_particle_rule(Projectile *p, int t) {
    if(t == EVENT_DEATH) {
        free_ref(p->args[3]);
    }

    if(t < 0) {
        return ACTION_ACK;
    }

    Laser *l = REF(p->args[3]);

    if(!l) {
        return ACTION_DESTROY;
    }

    p->pos = l->prule(l, 0);

    return ACTION_NONE;
}

static void elly_toe_laser_particle(Laser *l, cmplx origin) {
    Color *c = color_mul(COLOR_COPY(&l->color), &l->color);
    c->a = 0;

    PARTICLE(
        .sprite = "stardust",
        .pos = origin,
        .color = c,
        .rule = elly_toe_laser_particle_rule,
        .draw_rule = ScaleFade,
        .timeout = 30,
        .args = { 0, 0, 1.5 * I, add_ref(l) },
        .angle = M_PI*2*frand(),
    );

    PARTICLE(
        .sprite = "stain",
        .pos = origin,
        .color = color_mul_scalar(COLOR_COPY(c),0.5),
        .rule = elly_toe_laser_particle_rule,
        .draw_rule = ScaleFade,
        .timeout = 20,
        .args = { 0, 0, 1 * I, add_ref(l) },
        .angle = M_PI*2*frand(),
        .flags = PFLAG_REQUIREDPARTICLE,
    );

    PARTICLE(
        .sprite = "smoothdot",
        .pos = origin,
        .color = c,
        .rule = elly_toe_laser_particle_rule,
        .draw_rule = ScaleFade,
        .timeout = 40,
        .args = { 0, 0, 1, add_ref(l) },
        .angle = M_PI*2*frand(),
        .flags = PFLAG_REQUIREDPARTICLE,
    );
}

void elly_toe_laser_logic(Laser *l, int t) {
    if(t == l->deathtime) {
        static const int transfer[][4] = {
            {1, 0, 0, 0},
            {0,-1,-1, 3},
            {0,-1, 2, 3},
            {0, 3, 3, 1}
        };

        cmplx newpos = l->prule(l,t);
        if(creal(newpos) < 0 || cimag(newpos) < 0 || creal(newpos) > VIEWPORT_W || cimag(newpos) > VIEWPORT_H)
            return;

        int newtype, newtype2;
        do {
            newtype = 3.999999*frand();
            newtype2 = transfer[(int)(creal(l->args[1])+0.5)][newtype];
            // actually in this case, gluons are also always possible
            if(newtype2 == 1 && frand() > 0.5) {
                newtype = 2;
            }

            // I removed type 3 because i can’t draw dotted lines and it would be too difficult either way
        } while(newtype2 == -1 || newtype2 == 3 || newtype == 3);

        cmplx origin = l->prule(l,t);
        cmplx newdir = cexp(0.3*I);

        Laser *l1 = create_laser(origin,LASER_LENGTH,LASER_LENGTH,RGBA(1, 1, 1, 0),
            elly_toe_laser_pos,elly_toe_laser_logic,
            l->args[0]*newdir,
            newtype,
            LASER_EXTENT,
            0
        );

        Laser *l2 = create_laser(origin,LASER_LENGTH,LASER_LENGTH,RGBA(1, 1, 1, 0),
            elly_toe_laser_pos,elly_toe_laser_logic,
            l->args[0]/newdir,
            newtype2,
            LASER_EXTENT,
            0
        );

        elly_toe_laser_particle(l1, origin);
        elly_toe_laser_particle(l2, origin);
    }
    l->pos+=I*0.2;
}

