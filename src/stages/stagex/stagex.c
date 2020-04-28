/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "stagex.h"

#include "draw.h"
#include "background_anim.h"   // IWYU pragma: keep
#include "timeline.h"   // IWYU pragma: keep

static void stagex_begin(void) {
	stagex_drawsys_init();

	INVOKE_TASK(stagex_animate_background);
	INVOKE_TASK(stagex_timeline);
}

static void stagex_end(void) {
	stagex_drawsys_shutdown();
}

static void stagex_preload(ResourceGroup *rg) {
	res_group_preload(rg, RES_TEXTURE, RESF_DEFAULT,
		"cell_noise",
		"stageex/bg",
		"stageex/bg_binary",
		"stageex/code",
		"stageex/dissolve_mask",
	NULL);
	res_group_preload(rg, RES_SHADER_PROGRAM, RESF_DEFAULT,
		"copy_depth",
		"extra_bg",
		"extra_tower_apply_mask",
		"extra_tower_mask",
		"zbuf_fog",
	NULL);
	res_group_preload(rg, RES_MODEL, RESF_DEFAULT,
		"tower",
		"tower_alt_uv",
	NULL);
}

StageProcs stagex_procs = {
	.begin = stagex_begin,
	.end = stagex_end,
	.draw = stagex_draw_background,
	.preload = stagex_preload,
	.shader_rules = stagex_bg_effects,
	.postprocess_rules = stagex_postprocess_effects,
};
