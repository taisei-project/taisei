/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "spells.h"
#include "../elly.h"
#include "../extra.h"

#include "common_tasks.h"
#include "global.h"

static int baryon_broglie(Enemy *e, int t) {
    if(t < 0) {
        return 1;
    }

    if(!global.boss) {
        return ACTION_DESTROY;
    }

    int period = BROGLIE_PERIOD;
    int t_real = t;
    int attack_num = t_real / period;
    t %= period;

    TIMER(&t);
    Enemy *master = NULL;

    for(Enemy *en = global.enemies.first; en; en = en->next) {
		if(en->visual_rule == baryon) {
            master = en;
            break;
        }
    }

    assert(master != NULL);

    if(master != e) {
        if(t_real < period) {
            GO_TO(e, global.boss->pos, 0.03);
        } else {
            e->pos = global.boss->pos;
        }

        return 1;
    }

    int delay = 140;
    int step = 15;
    int cnt = 3;
    int fire_delay = 120;

    static double aim_angle;

    AT(delay) {
        elly_clap(global.boss,fire_delay);
        aim_angle = carg(e->pos - global.boss->pos);
    }

    FROM_TO(delay, delay + step * cnt - 1, step) {
        double a = 2*M_PI * (0.25 + 1.0/cnt*_i);
        cmplx n = cexp(I*a);
        double hue = (attack_num * M_PI + a + M_PI/6) / (M_PI*2);

        PROJECTILE(
            .proto = pp_ball,
            .pos = e->pos + 15*n,
            .color = HSLA(hue, 1.0, 0.55, 0.0),
            .rule = broglie_charge,
            .args = {
                n,
                (fire_delay - step * _i),
                attack_num,
                hue
            },
            .flags = PFLAG_NOCLEAR,
            .angle = (2*M_PI*_i)/cnt + aim_angle,
        );
    }

    if(t < delay /*|| t > delay + fire_delay*/) {
        cmplx target_pos = global.boss->pos + 100 * cexp(I*carg(global.plr.pos - global.boss->pos));
        GO_TO(e, target_pos, 0.03);
    }

    return 1;
}

void elly_broglie(Boss *b, int t) {
    TIMER(&t);

    AT(0) {
        set_baryon_rule(baryon_broglie);
    }

    AT(EVENT_DEATH) {
        set_baryon_rule(baryon_reset);
    }

    if(t < 0) {
        return;
    }

    int period = BROGLIE_PERIOD;
    double ofs = 100;

    cmplx positions[] = {
        VIEWPORT_W-ofs + ofs*I,
        VIEWPORT_W-ofs + ofs*I,
        ofs + (VIEWPORT_H-ofs)*I,
        ofs + ofs*I,
        ofs + ofs*I,
        VIEWPORT_W-ofs + (VIEWPORT_H-ofs)*I,
    };

    if(t/period > 0) {
        GO_TO(b, positions[(t/period) % (sizeof(positions)/sizeof(cmplx))], 0.02);
    }
}
