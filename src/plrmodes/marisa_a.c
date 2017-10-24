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

static void draw_laser_beam(complex src, complex dst, double size, double step, double t, Texture *tex, int u_length) {
    complex dir = dst - src;
    complex center = (src + dst) * 0.5;

    glPushMatrix();

    glTranslatef(creal(center), cimag(center), 0);
    glRotatef(180/M_PI*carg(dir), 0, 0, 1);
    glScalef(cabs(dir), size, 1);

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glTranslatef(-cimag(src) / step + t, 0, 0);
    glScalef(cabs(dir) / step, 1, 1);
    glMatrixMode(GL_MODELVIEW);

    glBindTexture(GL_TEXTURE_2D, tex->gltex);
    glUniform1f(u_length, cabs(dir) / step);
    draw_quad();

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);

    glPopMatrix();
}

static complex trace_laser(complex origin, complex vel, int damage) {
    complex target = origin;
    Projectile *p = NULL;

    create_projectile_p(&p, get_tex("proj/ball"), origin, 0, NULL, linear, vel, 0, 0, 0);
    p->type = PlrProj + damage;

    for(int t = 0; p; ++t) {
        int action = p->rule(p, t);
        int col = collision_projectile(p);

        if(col || action == ACTION_DESTROY || !projectile_in_viewport(p)) {
            complex nudge = -10 * vel / cabs(vel);
            target = p->pos + nudge;

            if(col) {
                tsrand_fill(3);
                create_particle2c("flare", target, 0, Shrink, timeout_linear, 3 + 5 * afrand(2), (2+afrand(0)*6)*cexp(I*M_PI*2*afrand(1)))->type=PlrProj;
            }

            delete_projectile(&p, p);
        }
    }

    return target;
}

static void set_color(int u_clr, Color c) {
    float ca[4];
    parse_color_array(c, ca);
    glUniform4fv(u_clr, 1, ca);
}

static float set_alpha(int u_alpha, float a) {
    if(a) {
        glUniform1f(u_alpha, a);
    }

    return a;
}

static float set_alpha_dimmed(int u_alpha, float a) {
    return set_alpha(u_alpha, a * a * 0.5);
}

static void draw_magic_star(complex pos, double a, Color c1, Color c2) {
    if(a <= 0) {
        return;
    }

    Color mul = rgba(1, 1, 1, a);
    c1 = multiply_colors(c1, mul);
    c2 = multiply_colors(c2, mul);

    Texture *tex = get_tex("part/magic_star");
    Shader *shader = get_shader("bullet_color");
    int u_clr = uniloc(shader, "color");
    glUseProgram(shader->prog);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glPushMatrix();
        glTranslatef(creal(pos), cimag(pos), -1);
        glScalef(0.25, 0.25, 1);
        glPushMatrix();
            set_color(u_clr, c1);
            glRotatef(global.frames * 3, 0, 0, 1);
            draw_texture_p(0, 0, tex);
        glPopMatrix();
        glPushMatrix();
            set_color(u_clr, c2);
            glRotatef(global.frames * -3, 0, 0, 1);
            draw_texture_p(0, 0, tex);
        glPopMatrix();
    glPopMatrix();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(0);
}

static void marisa_laser_slave_draw(Enemy *e, int t) {
    float laser_alpha = global.plr.slaves->args[0];
    float star_alpha = global.plr.slaves->args[1];

    draw_magic_star(e->pos, 0.75 * star_alpha,
        rgb(1.0, 0.1, 0.1),
        rgb(0.0, 0.1, 1.1)
    );

    marisa_common_slave_draw(e, t);

    if(laser_alpha <= 0) {
        return;
    }

    glColor4f(1, 1, 1, laser_alpha);
    glPushMatrix();
    glTranslatef(creal(e->args[3]), cimag(e->args[3]), 0);
    draw_texture(0, 0, "part/lasercurve");
    glPopMatrix();
    glColor4f(1, 1, 1, 1);
}

static void marisa_laser_fader_draw(Enemy *e, int t) {
}

static float get_laser_alpha(Enemy *e, float a) {
    if(e->draw_rule == marisa_laser_fader_draw) {
        return e->args[1];
    }

    return min(a, min(1, (global.frames - e->birthtime) * 0.1));
}

#define FOR_EACH_SLAVE(e) for(Enemy *e = global.plr.slaves; e; e = e->next) if(e != renderer)
#define FOR_EACH_REAL_SLAVE(e) FOR_EACH_SLAVE(e) if(e->draw_rule == marisa_laser_slave_draw)

static void marisa_laser_renderer_draw(Enemy *renderer, int t) {
    double a = creal(renderer->args[0]);
    Shader *shader = get_shader("marisa_laser");
    int u_clr = uniloc(shader, "color");
    int u_alpha = uniloc(shader, "alphamod");
    int u_length = uniloc(shader, "length");
    // int u_cutoff = uniloc(shader, "cutoff");
    Texture *tex0 = get_tex("part/marisa_laser0");
    Texture *tex1 = get_tex("part/marisa_laser1");

    glUseProgram(shader->prog);
    glUniform4f(u_clr, 1, 1, 1, 1);
    glBlendFunc(GL_SRC_COLOR, GL_ONE);
    glBindFramebuffer(GL_FRAMEBUFFER, resources.fbo.rgba[0].fbo);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(0, 0, 0, 1);
    glBlendEquation(GL_MAX);

    FOR_EACH_SLAVE(e) {
        if(set_alpha(u_alpha, get_laser_alpha(e, a))) {
            draw_laser_beam(e->pos, e->args[3], 32, 128, -0.02 * t, tex1, u_length);
        }
    }

    glBlendEquation(GL_FUNC_ADD);
    glBindFramebuffer(GL_FRAMEBUFFER, resources.fbo.fg[0].fbo);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(0);
    draw_fbo_viewport(&resources.fbo.rgba[0]);
    glUseProgram(shader->prog);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glUniform4f(u_clr, 1.0, 0.4, 0.0, 1.0);
    FOR_EACH_SLAVE(e) {
        if(set_alpha_dimmed(u_alpha, get_laser_alpha(e, a))) {
            draw_laser_beam(e->pos, e->args[3], 40, 128, t * -0.05, tex0, u_length);
        }
    }

    glUniform4f(u_clr, 0.1, 0.1, 1.0, 1.0);
    FOR_EACH_SLAVE(e) {
        if(set_alpha_dimmed(u_alpha, get_laser_alpha(e, a))) {
            draw_laser_beam(e->pos, e->args[3], 30, 128, t *  0.025, tex0, u_length);
        }
    }

    glUseProgram(0);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

static int marisa_laser_fader(Enemy *e, int t) {
    e->args[1] = approach(e->args[1], 0.0, 0.1);

    if(creal(e->args[1]) == 0) {
        return ACTION_DESTROY;
    }

    return 1;
}

static Enemy* spawn_laser_fader(Enemy *e, double alpha) {
    return create_enemy_p(&global.plr.slaves, e->pos, ENEMY_IMMUNE, marisa_laser_fader_draw, marisa_laser_fader,
        e->args[0], alpha, e->args[2], e->args[3]);
}

static int marisa_laser_renderer(Enemy *renderer, int t) {
    if(player_should_shoot(&global.plr, true) && renderer->next) {
        renderer->args[0] = approach(renderer->args[0], 1.0, 0.2);
        renderer->args[1] = approach(renderer->args[1], 1.0, 0.2);
        renderer->args[2] = 1;
    } else {
        if(creal(renderer->args[2])) {
            FOR_EACH_REAL_SLAVE(e) {
                spawn_laser_fader(e, renderer->args[0]);
            }

            renderer->args[2] = 0;
        }

        renderer->args[0] = 0;
        renderer->args[1] = approach(renderer->args[1], 0.0, 0.1);
    }

    return 1;
}

#undef FOR_EACH_SLAVE
#undef FOR_EACH_REAL_SLAVE

static int marisa_laser_slave(Enemy *e, int t) {
    if(t == EVENT_DEATH && !global.game_over && global.plr.slaves && creal(global.plr.slaves->args[0])) {
        spawn_laser_fader(e, global.plr.slaves->args[0]);
        return 1;
    }

    if(t < 0) {
        return 1;
    }

    e->pos = global.plr.pos + (1 - global.plr.focus/30.0)*e->pos0 + (global.plr.focus/30.0)*e->args[0];

    if(player_should_shoot(&global.plr, true)) {
        float angle = creal(e->args[2]);
        float f = smoothreclamp(global.plr.focus, 0, 30, 0, 1);
        f = smoothreclamp(f, 0, 1, 0, 1);
        float factor = (1.0 + 0.7 * psin(t/15.0)) * -(1-f) * !!angle;
        e->args[3] = trace_laser(e->pos, -5 * cexp(I*(angle*factor + M_PI/2)), creal(e->args[1]));
    }

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

static void marisa_laser_respawn_slaves(Player *plr, short npow) {
    Enemy *e = plr->slaves, *tmp;
    double dmg = 4;

    while(e != 0) {
        tmp = e;
        e = e->next;
        if(tmp->logic_rule == marisa_laser_slave) {
            delete_enemy(&plr->slaves, tmp);
        }
    }

    if(npow / 100 == 1) {
        create_enemy_p(&plr->slaves, -40.0*I, ENEMY_IMMUNE, marisa_laser_slave_draw, marisa_laser_slave, -40.0*I, dmg, 0, 0);
    }

    if(npow >= 200) {
        create_enemy_p(&plr->slaves, 25-5.0*I, ENEMY_IMMUNE, marisa_laser_slave_draw, marisa_laser_slave, 8-40.0*I, dmg,   -M_PI/30, 0);
        create_enemy_p(&plr->slaves, -25-5.0*I, ENEMY_IMMUNE, marisa_laser_slave_draw, marisa_laser_slave, -8-40.0*I, dmg,  M_PI/30, 0);
    }

    if(npow / 100 == 3) {
        create_enemy_p(&plr->slaves, -30.0*I, ENEMY_IMMUNE, marisa_laser_slave_draw, marisa_laser_slave, -50.0*I, dmg, 0, 0);
    }

    if(npow >= 400) {
        create_enemy_p(&plr->slaves, 17-30.0*I, ENEMY_IMMUNE, marisa_laser_slave_draw, marisa_laser_slave, 4-45.0*I, dmg, M_PI/60, 0);
        create_enemy_p(&plr->slaves, -17-30.0*I, ENEMY_IMMUNE, marisa_laser_slave_draw, marisa_laser_slave, -4-45.0*I, dmg, -M_PI/60, 0);
    }
}

static void marisa_laser_power(Player *plr, short npow) {
    if(plr->power / 100 == npow / 100) {
        return;
    }

    marisa_laser_respawn_slaves(plr, npow);
}

static void marisa_laser_init(Player *plr) {
    create_enemy_p(&plr->slaves, 0, ENEMY_IMMUNE, marisa_laser_renderer_draw, marisa_laser_renderer, 0, 0, 0, 0);
    marisa_laser_respawn_slaves(plr, plr->power);
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
        // "part/marilaser_part0",
        // "proj/marilaser",
        "proj/marisa",
        "masterspark",
        "masterspark_ring",
        "part/marisa_laser0",
        "part/marisa_laser1",
        "part/magic_star",
    NULL);

    preload_resources(RES_SHADER, flags,
        "marisa_laser",
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
        .init = marisa_laser_init,
    },
};
