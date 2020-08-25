/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "nonspells.h"
#include "../elly.h"

#include "global.h"

void elly_baryon_explode(Boss *b, int t) {
    TIMER(&t);

    GO_TO(b, BOSS_DEFAULT_GO_POS, 0.05);

    AT(20) {
        set_baryon_rule(baryon_explode);
    }

    AT(42) {
        audio_bgm_stop(1.0);
    }

    FROM_TO(0, 200, 1) {
        // tsrand_fill(2);
        // petal_explosion(1, b->pos + 100*afrand(0)*cexp(2.0*I*M_PI*afrand(1)));
        // global.shake_view = fmaxf(global.shake_view, 5 * _i / 200.0f);
        stage_shake_view(_i / 200.0f);

        if(_i > 30) {
            play_sfx_loop("charge_generic");
        }
    }

    AT(200) {
        tsrand_fill(2);
        stage_shake_view(40);
        play_sfx("boom");
        petal_explosion(100, b->pos + 100*afrand(0)*cexp(2.0*I*M_PI*afrand(1)));
        enemy_kill_all(&global.enemies);
    }
}
