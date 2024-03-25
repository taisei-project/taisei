/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "menu.h"
#include "global.h"
#include "video.h"
#include "eventloop/eventloop.h"
#include "replay/demoplayer.h"
#include "util/graphics.h"

MenuEntry *add_menu_entry(MenuData *menu, const char *name, MenuAction action, void *arg) {
	return dynarray_append(&menu->entries, {
		.action = action,
		.arg = arg,
		.transition = menu->transition,
		.name = name ? strdup(name) : NULL,
	});
}

void add_menu_separator(MenuData *menu) {
	dynarray_append(&menu->entries, {});
}

void free_menu(MenuData *menu) {
	if(menu == NULL) {
		return;
	}

	dynarray_foreach_elem(&menu->entries, MenuEntry *e, {
		mem_free(e->name);
	});

	dynarray_free_data(&menu->entries);
	mem_free(menu);
}

MenuData* alloc_menu(void) {
	return ALLOC(MenuData, {
		.selected = -1,
		.transition = TransFadeBlack,
		.transition_in_time = FADE_TIME,
		.transition_out_time = FADE_TIME,
		.fade = 1.0,
		.input = menu_input,
	});
}

void kill_menu(MenuData *menu) {
	if(menu != NULL) {
		menu->state = MS_Dead;
		// nani?!
	}
}

static void close_menu_finish(CallChainResult ccr) {
	MenuData *menu = ccr.ctx;

	if(TRANSITION_RESULT_CANCELED(ccr)) {
		return;
	}

	// This may happen with MF_AlwaysProcessInput menus, so make absolutely sure we
	// never run the call chain with menu->state == MS_Dead more than once.
	bool was_dead = (menu->state == MS_Dead);

	menu->state = MS_Dead;

	if(menu->selected != -1) {
		MenuEntry *e = dynarray_get_ptr(&menu->entries, menu->selected);

		if(e->action != NULL) {
			if(!(menu->flags & MF_Transient)) {
				menu->state = MS_Normal;
			}

			e->action(menu, e->arg);
		}
	}

	if(!was_dead) {
		run_call_chain(&menu->cc, menu);
	}
}

void close_menu(MenuData *menu) {
	TransitionRule trans = menu->transition;

	assert(menu->state != MS_Dead);
	menu->state = MS_FadeOut;

	if(menu->selected != -1) {
		trans = dynarray_get(&menu->entries, menu->selected).transition;
	}

	CallChain cc = CALLCHAIN(close_menu_finish, menu);

	if(trans) {
		set_transition(
			trans,
			menu->transition_in_time,
			menu->transition_out_time,
			cc
		);
	} else {
		run_call_chain(&cc, NULL);
	}
}

float menu_fade(MenuData *menu) {
	return transition.fade;
}

bool menu_input_handler(SDL_Event *event, void *arg) {
	MenuData *menu = arg;
	TaiseiEvent te = TAISEI_EVENT(event->type);

	switch(te) {
		case TE_MENU_CURSOR_DOWN: {
			play_sfx_ui("generic_shot");
			int orig_cursor = menu->cursor;
			do {
				if(++menu->cursor >= menu->entries.num_elements) {
					menu->cursor = 0;
				}

				if(menu->cursor == orig_cursor) {
					break;
				}
			} while(dynarray_get(&menu->entries, menu->cursor).action == NULL);

			return true;
		}

		case TE_MENU_CURSOR_UP: {
			play_sfx_ui("generic_shot");
			int orig_cursor = menu->cursor;
			do {
				if(--menu->cursor < 0) {
					menu->cursor = menu->entries.num_elements - 1;
				}

				if(menu->cursor == orig_cursor) {
					break;
				}
			} while(dynarray_get(&menu->entries, menu->cursor).action == NULL);

			return true;
		}

		case TE_MENU_ACCEPT:
			play_sfx_ui("shot_special1");
			if(dynarray_get(&menu->entries, menu->cursor).action) {
				menu->selected = menu->cursor;
				close_menu(menu);
			}

			return true;

		case TE_MENU_ABORT:
			play_sfx_ui("hit");
			if(menu->flags & MF_Abortable) {
				menu->selected = -1;
				close_menu(menu);
			}

			return true;

		default:
			return false;
	}
}

void menu_input(MenuData *menu) {
	events_poll((EventHandler[]){
		{ .proc = menu_input_handler, .arg = menu },
		{ NULL }
	}, EFLAG_MENU);
}

void menu_no_input(MenuData *menu) {
	events_poll(NULL, 0);
}

static LogicFrameAction menu_logic_frame(void *arg) {
	MenuData *menu = arg;

	if(menu->state == MS_Dead) {
		return LFRAME_STOP;
	}

	if(menu->logic) {
		menu->logic(menu);
	}

	menu->frames++;

	if(menu->state != MS_FadeOut || menu->flags & MF_AlwaysProcessInput) {
		assert(menu->input);
		menu->input(menu);
	} else {
		menu_no_input(menu);
	}

	update_transition();

	return LFRAME_WAIT;
}

static RenderFrameAction menu_render_frame(void *arg) {
	MenuData *menu = arg;
	assert(menu->draw);
	set_ortho(SCREEN_W, SCREEN_H);
	menu->draw(menu);
	draw_transition();
	return RFRAME_SWAP;
}

static void menu_end_loop(void *ctx) {
	MenuData *menu = ctx;

	if(menu->flags & MF_NoDemo) {
		demoplayer_resume();
	}

	if(menu->state != MS_Dead) {
		// definitely dead now...
		menu->state = MS_Dead;
		run_call_chain(&menu->cc, menu);
	}

	if(menu->end) {
		menu->end(menu);
	}

	free_menu(menu);
}

void enter_menu(MenuData *menu, CallChain next) {
	if(menu == NULL) {
		run_call_chain(&next, NULL);
		return;
	}

	menu->cc = next;

	if(menu->begin != NULL) {
		menu->begin(menu);
	}

	if(menu->flags & MF_NoDemo) {
		demoplayer_suspend();
	}

	eventloop_enter(menu, menu_logic_frame, menu_render_frame, menu_end_loop, FPS);
}
