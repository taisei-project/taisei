/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "hina.h"

#include "global.h"
#include "renderer/api.h"

void stage2_draw_hina_spellbg(Boss *h, int time) {
	SpriteParams sp = {};
	sp.pos.x = VIEWPORT_W/2;
	sp.pos.y = VIEWPORT_H/2;
	sp.scale.both = 0.6;
	sp.shader_ptr = res_shader("sprite_default");
	sp.sprite_ptr = res_sprite("stage2/spellbg1");
	sp.blend = BLEND_PREMUL_ALPHA;
	r_draw_sprite(&sp);
	sp.scale.both = 1;
	sp.blend = BLEND_MOD;
	sp.sprite_ptr = res_sprite("stage2/spellbg2");
	sp.rotation = (SpriteRotationParams) { .angle = time * 5 * DEG2RAD, .vector = { 0, 0, 1 } };
	r_draw_sprite(&sp);

	Animation *fireani = res_anim("fire");
	sp.sprite_ptr = animation_get_frame(fireani, get_ani_sequence(fireani, "main"), global.frames);
	sp.pos.x = re(h->pos);
	sp.pos.y = im(h->pos);
	sp.scale.both = 1;
	sp.rotation = (SpriteRotationParams) {};
	sp.blend = BLEND_PREMUL_ALPHA;
	r_draw_sprite(&sp);
}

Boss *stage2_spawn_hina(cmplx pos) {
	Boss *hina = create_boss(_("Kagiyama Hina"), "hina", pos);
	boss_set_portrait(hina, "hina", NULL, "normal");
	hina->glowcolor = *RGBA_MUL_ALPHA(0.7, 0.2, 0.3, 0.5);
	hina->shadowcolor = hina->glowcolor;
	return hina;
}
