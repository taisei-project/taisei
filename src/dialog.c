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
	d->state = DIALOG_STATE_IDLE;
	d->text.current = &d->text.buffers[0];
	d->text.fading_out = &d->text.buffers[1];
	COEVENT_INIT_ARRAY(d->events);
	return d;
}

void dialog_add_actor(Dialog *d, DialogActor *a, const char *name, DialogSide side) {
	memset(a, 0, sizeof(*a));
	a->name = name;
	a->face = "normal";
	a->side = side;
	a->target_opacity = 1;
	a->composite_dirty = true;

	if(side == DIALOG_SIDE_RIGHT) {
		a->speech_color = *RGB(0.6, 0.6, 1.0);
	} else {
		a->speech_color = *RGB(1.0, 1.0, 1.0);
	}

	alist_append(&d->actors, a);
}

void dialog_actor_set_face(DialogActor *a, const char *face) {
	a->face = face;
	a->composite_dirty = true;
}

void dialog_actor_set_variant(DialogActor *a, const char *variant) {
	a->variant = variant;
	a->composite_dirty = true;
}

void dialog_update(Dialog *d) {
	if(d->state == DIALOG_STATE_FADEOUT) {
		fapproach_asymptotic_p(&d->opacity, 0, 0.1, 1e-3);
	} else {
		fapproach_asymptotic_p(&d->opacity, 1, 0.05, 1e-3);
	}

	for(DialogActor *a = d->actors.first; a; a = a->next) {
		if(d->state == DIALOG_STATE_FADEOUT) {
			fapproach_asymptotic_p(&a->opacity, 0, 0.12, 1e-3);
		} else {
			fapproach_asymptotic_p(&a->opacity, a->target_opacity, 0.04, 1e-3);
		}
		fapproach_asymptotic_p(&a->focus, a->target_focus, 0.12, 1e-3);
	}

	fapproach_p(&d->text.current->opacity, 1, 1/120.0f);
	fapproach_p(&d->text.fading_out->opacity, 0, 1/60.0f);
}

void dialog_skippable_wait(Dialog *d, int timeout) {
	CoEventSnapshot snap = coevent_snapshot(&d->events.skip_requested);

	assert(d->state == DIALOG_STATE_IDLE);
	d->state = DIALOG_STATE_WAITING_FOR_SKIP;

	while(timeout > 0) {
		dialog_update(d);

		--timeout;
		YIELD;

		if(coevent_poll(&d->events.skip_requested, &snap) != CO_EVENT_PENDING) {
			log_debug("Skipped with %i remaining", timeout);
			break;
		}
	}

	assert(d->state == DIALOG_STATE_WAITING_FOR_SKIP);
	d->state = DIALOG_STATE_IDLE;

	if(timeout == 0) {
		log_debug("Timed out");
	}
}

int dialog_util_estimate_wait_timeout_from_text(const char *text) {
	return 1800;
}

static void dialog_set_text(Dialog *d, const char *text, const Color *clr) {
	DialogTextBuffer *temp = d->text.current;
	d->text.current = d->text.fading_out;
	d->text.fading_out = temp;

	d->text.current->color = *clr;
	d->text.current->text = text;
}

void dialog_focus_actor(Dialog *d, DialogActor *actor) {
	for(DialogActor *a = d->actors.first; a; a = a->next) {
		a->target_focus = 0;
	}

	actor->target_focus = 1;

	// make focused actor drawn on top of everyone else
	alist_unlink(&d->actors, actor);
	alist_append(&d->actors, actor);
}

void dialog_message_ex(Dialog *d, const DialogMessageParams *params) {
	assume(params->actor != NULL);
	assume(params->text != NULL);

	log_debug("%s: %s", params->actor->name, params->text);

	dialog_set_text(d, params->text, &params->actor->speech_color);
	dialog_focus_actor(d, params->actor);

	if(params->implicit_wait) {
		assume(params->wait_timeout > 0);

		if(params->wait_skippable) {
			dialog_skippable_wait(d, params->wait_timeout);
		} else {
			WAIT(params->wait_timeout);
		}
	}
}

void dialog_message(Dialog *d, DialogActor *actor, const char *text) {
	DialogMessageParams p = { 0 };
	p.actor = actor;
	p.text = text;
	p.implicit_wait = true;
	p.wait_skippable = true;
	p.wait_timeout = dialog_util_estimate_wait_timeout_from_text(text);
	dialog_message_ex(d, &p);
}

void dialog_end(Dialog *d) {
	d->state = DIALOG_STATE_FADEOUT;
	coevent_signal(&d->events.fadeout_began);

	for(DialogActor *a = d->actors.first; a; a = a->next) {
		a->target_opacity = 0;
	}

	wait_for_fadeout: {
		if(d->opacity > 0) {
			YIELD;
			goto wait_for_fadeout;
		}

		for(DialogActor *a = d->actors.first; a; a = a->next) {
			if(a->opacity > 0) {
				YIELD;
				goto wait_for_fadeout;
			}
		}
	}

	coevent_signal(&d->events.fadeout_ended);
	cotask_cancel(cotask_active());
	UNREACHABLE;
}

static void dialog_actor_update_composite(DialogActor *a) {
	assume(a->name != NULL);
	assume(a->face != NULL);

	if(!a->composite_dirty) {
		return;
	}

	log_debug("%s (%p) is dirty", a->name, (void*)a);

	Sprite *spr_base, *spr_face;

	size_t name_len = strlen(a->name);
	size_t face_len = strlen(a->face);
	size_t variant_len;

	size_t lenfull_base = sizeof("dialog/") + name_len - 1;
	size_t lenfull_face = lenfull_base + sizeof("_face_") + face_len - 1;

	if(a->variant) {
		variant_len = strlen(a->variant);
		lenfull_base += sizeof("_variant_") + variant_len - 1;
	}

	char buf[imax(lenfull_base, lenfull_face) + 1];
	char *dst = buf;
	dst = memcpy(dst, "dialog/", sizeof("dialog/") - 1);
	dst = memcpy(dst + sizeof("dialog/") - 1, a->name, name_len + 1);

	if(a->variant) {
		char *tmp = dst;
		dst = memcpy(dst + name_len, "_variant_", sizeof("_variant_") - 1);
		dst = memcpy(dst + sizeof("_variant_") - 1, a->variant, variant_len + 1);
		dst = tmp;
	}

	spr_base = get_sprite(buf);
	log_debug("base: %s", buf);
	assume(spr_base != NULL);

	dst = memcpy(dst + name_len, "_face_", sizeof("_face_") - 1);
	dst = memcpy(dst + sizeof("_face_") - 1, a->face, face_len + 1);

	spr_face = get_sprite(buf);
	log_debug("face: %s", buf);
	assume(spr_face != NULL);

	if(a->composite.tex != NULL) {
		log_debug("destroyed texture at %p", (void*)a->composite.tex);
		r_texture_destroy(a->composite.tex);
	}

	render_character_portrait(spr_base, spr_face, &a->composite);
	log_debug("created texture at %p", (void*)a->composite.tex);
	a->composite_dirty = false;
}

void dialog_destroy(Dialog *d) {
	COEVENT_CANCEL_ARRAY(d->events);

	for(DialogActor *a = d->actors.first; a; a = a->next) {
		if(a->composite.tex) {
			r_texture_destroy(a->composite.tex);
		}
	}

	free(d);
}

void dialog_draw(Dialog *dialog) {
	if(dialog == NULL) {
		return;
	}

	float o = dialog->opacity;

	for(DialogActor *a = dialog->actors.first; a; a = a->next) {
		dialog_actor_update_composite(a);
	}

	r_state_push();
	r_state_push();
	r_shader("sprite_default");

	r_mat_mv_push();
	r_mat_mv_translate(VIEWPORT_X, 0, 0);

	const double dialog_width = VIEWPORT_W * 1.2;

	r_mat_mv_push();
	r_mat_mv_translate(dialog_width/2.0, 64, 0);

	Color clr = { 0 };

	for(DialogActor *a = dialog->actors.first; a; a = a->next) {
		if(a->opacity <= 0) {
			continue;
		}

		dialog_actor_update_composite(a);
		Sprite *portrait = &a->composite;
		assume(portrait->tex != NULL);

		float portrait_w = sprite_padded_width(portrait);
		float portrait_h = sprite_padded_height(portrait);

		r_mat_mv_push();

		if(a->side == DIALOG_SIDE_LEFT) {
			r_cull(CULL_FRONT);
			r_mat_mv_scale(-1, 1, 1);
		} else {
			r_cull(CULL_BACK);
		}

		if(a->opacity < 1) {
			r_mat_mv_translate(120 * (1 - a->opacity), 0, 0);
		}

		float ofs = 10 * (1 - a->focus);
		r_mat_mv_translate(ofs, ofs, 0);
		float brightness = 0.5 + 0.5 * a->focus;
		clr.r = clr.g = clr.b = brightness;
		clr.a = 1;

		color_mul_scalar(&clr, a->opacity);

		r_flush_sprites();
		r_draw_sprite(&(SpriteParams) {
			.blend = BLEND_PREMUL_ALPHA,
			.color = &clr,
			.pos.x = (dialog_width - portrait_w) / 2 + 32 + a->offset.x,
			.pos.y = VIEWPORT_H - portrait_h / 2 + a->offset.y,
			.sprite_ptr = portrait,
		});

		r_mat_mv_pop();
	}

	r_mat_mv_pop();
	r_state_pop();

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

	dialog_bg_rect.w = VIEWPORT_W * 0.86;
	dialog_bg_rect.x -= dialog_bg_rect.w * 0.5;
	dialog_bg_rect.y -= dialog_bg_rect.h * 0.5;

	if(dialog->text.fading_out->opacity > 0) {
		clr = dialog->text.fading_out->color;
		color_mul_scalar(&clr, o);

		text_draw_wrapped(dialog->text.fading_out->text, dialog_bg_rect.w, &(TextParams) {
			.shader = "text_dialog",
			.aux_textures = { get_tex("cell_noise") },
			.shader_params = &(ShaderCustomParams) {{ o * (1.0 - (0.2 + 0.8 * (1 - dialog->text.fading_out->opacity))), 1 }},
			.color = &clr,
			.pos = { VIEWPORT_W/2, VIEWPORT_H-110 + font_get_lineskip(font) },
			.align = ALIGN_CENTER,
			.font_ptr = font,
			.overlay_projection = &dialog_bg_rect,
		});
	}

	if(dialog->text.current->opacity > 0) {
		clr = dialog->text.current->color;
		color_mul_scalar(&clr, o);

		text_draw_wrapped(dialog->text.current->text, dialog_bg_rect.w, &(TextParams) {
			.shader = "text_dialog",
			.aux_textures = { get_tex("cell_noise") },
			.shader_params = &(ShaderCustomParams) {{ o * dialog->text.current->opacity, 0 }},
			.color = &clr,
			.pos = { VIEWPORT_W/2, VIEWPORT_H-110 + font_get_lineskip(font) },
			.align = ALIGN_CENTER,
			.font_ptr = font,
			.overlay_projection = &dialog_bg_rect,
		});
	}

	r_mat_tex_pop();
	r_mat_mv_pop();
	r_mat_mv_pop();
	r_state_pop();
}

bool dialog_page(Dialog *d) {
	if(d->state == DIALOG_STATE_WAITING_FOR_SKIP) {
		coevent_signal(&d->events.skip_requested);
		return true;
	}

	return false;
}

bool dialog_is_active(Dialog *d) {
	return d && d->state != DIALOG_STATE_FADEOUT;
}

void dialog_preload(void) {
	preload_resource(RES_SHADER_PROGRAM, "text_dialog", RESF_DEFAULT);
}
