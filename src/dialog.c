/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "dialog.h"
#include "global.h"

Dialog *dialog_create(void) {
	Dialog *d = calloc(1, sizeof(Dialog));
	d->page_time = global.frames;
	d->birthtime = global.frames;
	return d;
}

void dialog_set_image(Dialog *d, DialogSide side, const char *img) {
	d->images[side] = img ? get_sprite(img) : NULL;
}

static int message_index(Dialog *d, int offset) {
	int idx = d->pos + offset;

	if(idx >= d->count) {
		idx = d->count - 1;
	}

	while(
		idx >= 0 &&
		d->actions[idx].type != DIALOG_MSG_LEFT &&
		d->actions[idx].type != DIALOG_MSG_RIGHT
	) {
		--idx;
	}

	return idx;
}

DialogAction * dialog_add_action(Dialog *d, DialogActionType side, const char *msg) {
	d->actions = realloc(d->actions, (++d->count)*sizeof(DialogAction));
	d->actions[d->count-1].type = side;
	d->actions[d->count-1].msg = malloc(strlen(msg) + 1);
	d->actions[d->count-1].timeout = 0;
	strlcpy(d->actions[d->count-1].msg, msg, strlen(msg) + 1);
	return &d->actions[d->count-1];
}

void dialog_destroy(Dialog *d) {
	for(int i = 0; i < d->count; i++) {
		free(d->actions[i].msg);
	}

	free(d->actions);
	free(d);
}

void dialog_draw(Dialog *dialog) {
	if(dialog == NULL) {
		return;
	}

	float o = dialog->opacity;

	if(o == 0) {
		return;
	}

	r_state_push();
	r_state_push();
	r_shader("sprite_default");

	r_mat_push();
	r_mat_translate(VIEWPORT_X, 0, 0);

	const double dialog_width = VIEWPORT_W * 1.2;

	r_mat_push();
	r_mat_translate(dialog_width/2.0, 64, 0);

	int cur_idx = message_index(dialog, 0);
	int pre_idx = message_index(dialog, -1);

	assume(cur_idx >= 0);

	int cur_side = dialog->actions[cur_idx].type;
	int pre_side = pre_idx >= 0 ? dialog->actions[pre_idx].type : 2;

	Color clr = { 0 };

	const float page_time = 10;
	float page_alpha = min(global.frames - (dialog->page_time), page_time) / page_time;

	const float page_text_time = 60;
	float page_text_alpha = min(global.frames - dialog->page_time, page_text_time) / page_text_time;

	int loop_start = 1;
	int loop_incr = 1;

	if(cur_side == 0) {
		loop_start = 1;
		loop_incr = -1;
	} else {
		loop_start = 0;
		loop_incr = 1;
	}

	for(int i = loop_start; i < 2 && i >= 0; i += loop_incr) {
		Sprite *portrait = dialog->images[i];

		if(!portrait) {
			continue;
		}

		r_mat_push();

		if(i == DIALOG_MSG_LEFT) {
			r_cull(CULL_FRONT);
			r_mat_scale(-1, 1, 1);
		} else {
			r_cull(CULL_BACK);
		}

		if(o < 1) {
			r_mat_translate(120 * (1 - o), 0, 0);
		}

		float dir = (1 - 2 * (i == cur_side));
		float ofs = 10 * dir;

		if(page_alpha < 10 && ((i != pre_side && i == cur_side) || (i == pre_side && i != cur_side))) {
			r_mat_translate(ofs * page_alpha, ofs * page_alpha, 0);
			float brightness = min(1.0 - 0.7 * page_alpha * dir, 1);
			clr.r = clr.g = clr.b = brightness;
			clr.a = 1;
		} else {
			r_mat_translate(ofs, ofs, 0);
			clr = *RGB(1 - (dir > 0) * 0.7, 1 - (dir > 0) * 0.7, 1 - (dir > 0) * 0.7);
		}

		color_mul_scalar(&clr, o);

		SpriteParams sp = { 0 };
		sp.blend = BLEND_PREMUL_ALPHA;
		sp.color = &clr;
		sp.pos.x = (dialog_width - portrait->w) / 2 + 32;
		sp.pos.y = VIEWPORT_H - portrait->h / 2;
		sp.sprite_ptr = portrait;
		r_draw_sprite(&sp);

		if(portrait) {
			sp.sprite_ptr = portrait;
			r_draw_sprite(&sp);
		}

		r_mat_pop();
	}

	r_mat_pop();
	r_state_pop();

	o *= smooth(clamp((global.frames - dialog->birthtime - 10) / 30.0, 0, 1));

	FloatRect dialog_bg_rect = {
		.extent = { VIEWPORT_W-40, 110 },
		.offset = { VIEWPORT_W/2, VIEWPORT_H-55 },
	};

	r_mat_push();
	if(o < 1) {
		r_mat_translate(0, 100 * (1 - o), 0);
	}
	r_color4(0, 0, 0, 0.8 * o);
	r_mat_push();
	r_mat_translate(dialog_bg_rect.x, dialog_bg_rect.y, 0);
	r_mat_scale(dialog_bg_rect.w, dialog_bg_rect.h, 1);
	r_shader_standard_notex();
	r_draw_quad();
	r_mat_pop();

	Font *font = get_font("standard");

	r_mat_mode(MM_TEXTURE);
	r_mat_push();
	// r_mat_scale(2, 0.2, 0);
	// r_mat_translate(0, -global.frames/page_text_time, 0);
	r_mat_mode(MM_MODELVIEW);

	dialog_bg_rect.w = VIEWPORT_W * 0.86;
	dialog_bg_rect.x -= dialog_bg_rect.w * 0.5;
	dialog_bg_rect.y -= dialog_bg_rect.h * 0.5;
	// dialog_bg_rect.h = dialog_bg_rect.w;

	if(pre_idx >= 0 && page_text_alpha < 1) {
		if(pre_side == DIALOG_MSG_RIGHT) {
			clr = *RGB(0.6, 0.6, 1.0);
		} else {
			clr = *RGB(1.0, 1.0, 1.0);
		}

		color_mul_scalar(&clr, o);

		text_draw_wrapped(dialog->actions[pre_idx].msg, VIEWPORT_W * 0.86, &(TextParams) {
			.shader = "text_dialog",
			.aux_textures = { get_tex("cell_noise") },
			.shader_params = &(ShaderCustomParams) {{ o * (1.0 - (0.2 + 0.8 * page_text_alpha)), 1 }},
			.color = &clr,
			.pos = { VIEWPORT_W/2, VIEWPORT_H-110 + font_get_lineskip(font) },
			.align = ALIGN_CENTER,
			.font_ptr = font,
			.overlay_projection = &dialog_bg_rect,
		});
	}

	if(cur_side == DIALOG_MSG_RIGHT) {
		clr = *RGB(0.6, 0.6, 1.0);
	} else {
		clr = *RGB(1.0, 1.0, 1.0);
	}

	color_mul_scalar(&clr, o);

	text_draw_wrapped(dialog->actions[cur_idx].msg, VIEWPORT_W * 0.86, &(TextParams) {
		.shader = "text_dialog",
		.aux_textures = { get_tex("cell_noise") },
		.shader_params = &(ShaderCustomParams) {{ o * page_text_alpha, 0 }},
		.color = &clr,
		.pos = { VIEWPORT_W/2, VIEWPORT_H-110 + font_get_lineskip(font) },
		.align = ALIGN_CENTER,
		.font_ptr = font,
		.overlay_projection = &dialog_bg_rect,
	});

	r_mat_mode(MM_TEXTURE);
	r_mat_pop();
	r_mat_mode(MM_MODELVIEW);

	r_mat_pop();
	r_mat_pop();
	r_state_pop();
}

bool dialog_page(Dialog **d) {
	if(!*d || (*d)->pos >= (*d)->count) {
		return false;
	}

	int to = (*d)->actions[(*d)->pos].timeout;

	if(to && to > global.frames) {
		return false;
	}

	(*d)->pos++;
	(*d)->page_time = global.frames;

	if((*d)->pos >= (*d)->count) {
		// XXX: maybe this can be handled elsewhere?
		if(!global.boss)
			global.timer++;
	} else if((*d)->actions[(*d)->pos].type == DIALOG_SET_BGM) {
		stage_start_bgm((*d)->actions[(*d)->pos].msg);
		return dialog_page(d);
	}

	return true;
}

void dialog_update(Dialog **d) {
	if(!*d) {
		return;
	}

	if(dialog_is_active(*d)) {
		int to = (*d)->actions[(*d)->pos].timeout;

		if(
			(to && to >= global.frames) ||
			((global.plr.inputflags & INFLAG_SKIP) && global.frames - (*d)->page_time > 3)
		) {
			dialog_page(d);
		}
	}

	// important to check this again; the page_dialog call may have ended the dialog

	if(dialog_is_active(*d)) {
		fapproach_asymptotic_p(&(*d)->opacity, 1, 0.05, 1e-3);
	} else {
		fapproach_asymptotic_p(&(*d)->opacity, 0, 0.1, 1e-3);
		if((*d)->opacity == 0) {
			dialog_destroy(*d);
			*d = NULL;
		}
	}
}

bool dialog_is_active(Dialog *d) {
	return d && (d->pos < d->count);
}
