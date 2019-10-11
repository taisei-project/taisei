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

void dialog_set_base(Dialog *d, DialogSide side, const char *sprite) {
	d->spr_base[side] = sprite ? get_sprite(sprite) : NULL;
	d->valid_composites &= ~(1 << side);
}

void dialog_set_base_p(Dialog *d, DialogSide side, Sprite *sprite) {
	d->spr_base[side] = sprite;
	d->valid_composites &= ~(1 << side);
}

void dialog_set_face(Dialog *d, DialogSide side, const char *sprite) {
	d->spr_face[side] = sprite ? get_sprite(sprite) : NULL;
	d->valid_composites &= ~(1 << side);
}

void dialog_set_face_p(Dialog *d, DialogSide side, Sprite *sprite) {
	d->spr_face[side] = sprite;
	d->valid_composites &= ~(1 << side);
}

void dialog_set_char(Dialog *d, DialogSide side, const char *char_name, const char *char_face, const char *char_variant) {
	size_t name_len = strlen(char_name);
	size_t face_len = strlen(char_face);
	size_t variant_len;

	size_t lenfull_base = sizeof("dialog/") + name_len - 1;
	size_t lenfull_face = lenfull_base + sizeof("_face_") + face_len - 1;

	if(char_variant) {
		variant_len = strlen(char_variant);
		lenfull_base += sizeof("_variant_") + variant_len - 1;
	}

	char buf[imax(lenfull_base, lenfull_face) + 1];
	char *dst = buf;
	char *variant_dst;
	dst = memcpy(dst, "dialog/", sizeof("dialog/") - 1);
	dst = memcpy(dst + sizeof("dialog/") - 1, char_name, name_len + 1);

	if(char_variant) {
		variant_dst = dst + name_len;
	} else {
		d->spr_base[side] = get_sprite(buf);
	}

	dst = memcpy(dst + name_len, "_face_", sizeof("_face_") - 1);
	dst = memcpy(dst + sizeof("_face_") - 1, char_face, face_len + 1);
	d->spr_face[side] = get_sprite(buf);

	if(!char_variant) {
		return;
	}

	dst = memcpy(variant_dst, "_variant_", sizeof("_variant_") - 1);
	dst = memcpy(dst + sizeof("_variant_") - 1, char_variant, variant_len + 1);
	d->spr_base[side] = get_sprite(buf);

	d->valid_composites &= ~(1 << side);
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

DialogAction *dialog_add_action(Dialog *d, const DialogAction *action) {
	d->actions = realloc(d->actions, (++d->count)*sizeof(DialogAction));
	d->actions[d->count - 1] = *action;
	return d->actions + d->count - 1;
}

static void update_composite(Dialog *d, DialogSide side) {
	if(d->valid_composites & (1 << side)) {
		return;
	}

	Sprite *composite = d->spr_composite + side;
	Sprite *spr_base = d->spr_base[side];
	Sprite *spr_face = d->spr_face[side];

	if(composite->tex != NULL) {
		r_texture_destroy(composite->tex);
	}

	if(spr_base != NULL) {
		assert(spr_face != NULL);
		render_character_portrait(spr_base, spr_face, composite);
	} else {
		composite->tex = NULL;
	}

	d->valid_composites |= (1 << side);
}

static void update_composites(Dialog *d) {
	update_composite(d, DIALOG_LEFT);
	update_composite(d, DIALOG_RIGHT);
}

void dialog_destroy(Dialog *d) {
	memset(&d->spr_base, 0, sizeof(d->spr_base));
	memset(&d->spr_face, 0, sizeof(d->spr_face));
	d->valid_composites = 0;
	update_composites(d);
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

	update_composites(dialog);

	r_state_push();
	r_state_push();
	r_shader("sprite_default");

	r_mat_mv_push();
	r_mat_mv_translate(VIEWPORT_X, 0, 0);

	const double dialog_width = VIEWPORT_W * 1.2;

	r_mat_mv_push();
	r_mat_mv_translate(dialog_width/2.0, 64, 0);

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
		Sprite *portrait = dialog->spr_composite + i;

		if(portrait->tex == NULL) {
			continue;
		}

		float portrait_w = sprite_padded_width(portrait);
		float portrait_h = sprite_padded_height(portrait);

		r_mat_mv_push();

		if(i == DIALOG_MSG_LEFT) {
			r_cull(CULL_FRONT);
			r_mat_mv_scale(-1, 1, 1);
		} else {
			r_cull(CULL_BACK);
		}

		if(o < 1) {
			r_mat_mv_translate(120 * (1 - o), 0, 0);
		}

		float dir = (1 - 2 * (i == cur_side));
		float ofs = 10 * dir;

		if(page_alpha < 10 && ((i != pre_side && i == cur_side) || (i == pre_side && i != cur_side))) {
			r_mat_mv_translate(ofs * page_alpha, ofs * page_alpha, 0);
			float brightness = min(1.0 - 0.5 * page_alpha * dir, 1);
			clr.r = clr.g = clr.b = brightness;
			clr.a = 1;
		} else {
			r_mat_mv_translate(ofs, ofs, 0);
			clr = *RGB(1 - (dir > 0) * 0.5, 1 - (dir > 0) * 0.5, 1 - (dir > 0) * 0.5);
		}

		color_mul_scalar(&clr, o);

		r_draw_sprite(&(SpriteParams) {
			.blend = BLEND_PREMUL_ALPHA,
			.color = &clr,
			.pos.x = (dialog_width - portrait_w) / 2 + 32,
			.pos.y = VIEWPORT_H - portrait_h / 2,
			.sprite_ptr = portrait,
		});

		r_mat_mv_pop();
	}

	r_mat_mv_pop();
	r_state_pop();

	o *= smooth(clamp((global.frames - dialog->birthtime - 10) / 30.0, 0, 1));

	FloatRect dialog_bg_rect = {
		.extent = { VIEWPORT_W-40, 110 },
		.offset = { VIEWPORT_W/2, VIEWPORT_H-55 },
	};

	r_mat_mv_push();
	if(o < 1) {
		r_mat_mv_translate(0, 100 * (1 - o), 0);
	}
	r_color4(0, 0, 0, 0.8 * o);
	r_mat_mv_push();
	r_mat_mv_translate(dialog_bg_rect.x, dialog_bg_rect.y, 0);
	r_mat_mv_scale(dialog_bg_rect.w, dialog_bg_rect.h, 1);
	r_shader_standard_notex();
	r_draw_quad();
	r_mat_mv_pop();

	Font *font = get_font("standard");

	r_mat_tex_push();
	// r_mat_tex_scale(2, 0.2, 0);
	// r_mat_tex_translate(0, -global.frames/page_text_time, 0);

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

		text_draw_wrapped(dialog->actions[pre_idx].data, VIEWPORT_W * 0.86, &(TextParams) {
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

	text_draw_wrapped(dialog->actions[cur_idx].data, VIEWPORT_W * 0.86, &(TextParams) {
		.shader = "text_dialog",
		.aux_textures = { get_tex("cell_noise") },
		.shader_params = &(ShaderCustomParams) {{ o * page_text_alpha, 0 }},
		.color = &clr,
		.pos = { VIEWPORT_W/2, VIEWPORT_H-110 + font_get_lineskip(font) },
		.align = ALIGN_CENTER,
		.font_ptr = font,
		.overlay_projection = &dialog_bg_rect,
	});

	r_mat_tex_pop();
	r_mat_mv_pop();
	r_mat_mv_pop();
	r_state_pop();
}

bool dialog_page(Dialog **pdialog) {
	Dialog *d = *pdialog;

	if(!d || d->pos >= d->count) {
		return false;
	}

	int to = d->actions[d->pos].timeout;

	if(to && to > global.frames) {
		assert(d->actions[d->pos].type == DIALOG_MSG_LEFT || d->actions[d->pos].type == DIALOG_MSG_RIGHT);
		return false;
	}

	DialogAction *a = d->actions + d->pos;
	d->pos++;
	d->page_time = global.frames;

	if(d->pos >= d->count) {
		// XXX: maybe this can be handled elsewhere?
		if(!global.boss)
			global.timer++;
	}

	switch(a->type) {
		case DIALOG_SET_BGM:
			stage_start_bgm(a->data);
			break;

		case DIALOG_SET_FACE_RIGHT:
		case DIALOG_SET_FACE_LEFT: {
			DialogSide side = (
				a->type == DIALOG_SET_FACE_RIGHT
					? DIALOG_RIGHT
					: DIALOG_LEFT
			);

			dialog_set_face(d, side, a->data);
			break;
		}

		default:
			break;
	}

	return true;
}

void dialog_update(Dialog **d) {
	if(!*d) {
		return;
	}

	if(dialog_is_active(*d)) {
		while((*d)->pos < (*d)->count) {
			if(
				(*d)->actions[(*d)->pos].type != DIALOG_MSG_LEFT &&
				(*d)->actions[(*d)->pos].type != DIALOG_MSG_RIGHT
			) {
				dialog_page(d);
			} else {
				int to = (*d)->actions[(*d)->pos].timeout;

				if(
					(to && to >= global.frames) ||
					((global.plr.inputflags & INFLAG_SKIP) && global.frames - (*d)->page_time > 3)
				) {
					dialog_page(d);
				}

				break;
			}
		}
	}

	// important to check this again; the dialog_page call may have ended the dialog

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

void dialog_preload(void) {
	preload_resource(RES_SHADER_PROGRAM, "text_dialog", RESF_DEFAULT);
}
