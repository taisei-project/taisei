/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "events.h"
#include "config.h"
#include "global.h"
#include "video.h"
#include "gamepad.h"

static uint32_t keyrepeat_paused_until;
static ListContainer *global_handlers;

uint32_t sdl_first_user_event;

static void events_register_default_handlers(void);
static void events_unregister_default_handlers(void);

/*
 *	Public API
 */

void events_init(void) {
	sdl_first_user_event = SDL_RegisterEvents(NUM_TAISEI_EVENTS);

	if(sdl_first_user_event == (uint32_t)-1) {
		char *s =
			"You have exhausted the SDL userevent pool. "
			"How you managed that is beyond me, but congratulations. "
#if (LOG_DEFAULT_LEVELS_BACKTRACE & LOG_FATAL)
			"Here's your prize stack trace."
#endif
			;
		log_fatal("%s", s);
	}

	SDL_EventState(SDL_MOUSEMOTION, SDL_DISABLE);

	events_register_default_handlers();
}

void events_shutdown(void) {
	events_unregister_default_handlers();

#ifdef DEBUG
	if(global_handlers) {
		log_warn(
			"Someone didn't unregister their handler. "
			"Clean up after yourself, I'm not your personal maid. "
			"Hint: ASan or valgrind can probably determine the culprit."
		);
	}
#endif
}

static bool events_invoke_handler(SDL_Event *event, EventHandler *handler) {
	assert(handler->proc != NULL);

	if(!handler->event_type || handler->event_type == event->type) {
		bool result = handler->proc(event, handler->arg);

		return result;
	}

	return false;
}

static int handler_container_prio_func(void *h) {
	return ((EventHandler*)((ListContainer*)h)->data)->priority;
}

static EventPriority real_priority(EventPriority prio) {
	if(prio == EPRIO_DEFAULT) {
		return EPRIO_DEFAULT_REMAP;
	}

	assert(prio < NUM_EPRIOS);
	return prio;
}

static void events_print_handlers(ListContainer *list) {
	for(ListContainer *c = list; c; c = c->next) {
		EventHandler *h = c->data;
		log_debug(" * %p %u", *(void**)&h->proc, h->priority);
	}
}

static bool events_invoke_handlers(SDL_Event *event, ListContainer *h_list, EventHandler *h_array) {
	// invoke handlers from two sources (a list and an array) in the correct order according to priority
	// list items take precedence
	//
	// assumptions:
	// 		h_list is sorted by priority
	//		h_array is in arbitrary order and terminated with a NULL-proc entry

	bool result = false;

	if(h_list && !h_array) {
		// case 1 (simplest): we have a list and no custom handlers

		for(ListContainer *c = h_list; c; c = c->next) {
			if(result = events_invoke_handler(event, (EventHandler*)c->data)) {
				break;
			}
		}

		return result;
	}

	if(h_list && h_array) {
		// case 2 (suboptimal): we have both a list and a disordered array; need to do some actual work
		// if you want to optimize this be my guest

		ListContainer *merged_list = NULL;
		ListContainer *prevc = NULL;

		// copy the list
		for(ListContainer *c = h_list; c; c = c->next) {
			ListContainer *newc = calloc(1, sizeof(ListContainer));
			newc->data = c->data;
			newc->prev = prevc;

			if(prevc) {
				prevc->next = newc;
			}

			if(!merged_list) {
				merged_list = newc;
			}

			prevc = newc;
		}

		// merge the array into the list copy, respecting priority
		for(EventHandler *h = h_array; h->proc; ++h) {
			create_container_at_priority(
				&merged_list,
				real_priority(h->priority),
				handler_container_prio_func
			)->data = h;
		}

		// iterate over the merged list
		for(ListContainer *c = merged_list; c; c = c->next) {
			if(result = events_invoke_handler(event, (EventHandler*)c->data)) {
				break;
			}
		}

		delete_all_elements((void**)&merged_list, delete_element);
		return result;
	}

	if(!h_list && h_array) {
		// case 3 (unlikely): we don't have a list for some reason (no global handlers?), but there are custom handlers

		for(EventHandler *h = h_array; h->proc; ++h) {
			if(result = events_invoke_handler(event, h)) {
				break;
			}
		}

		return result;
	}

	// case 4 (unlikely): huh? okay then
	return result;
}

void events_register_handler(EventHandler *handler) {
	assert(handler->proc != NULL);
	EventHandler *handler_alloc = malloc(sizeof(EventHandler));
	memcpy(handler_alloc, handler, sizeof(EventHandler));

	if(handler_alloc->priority == EPRIO_DEFAULT) {
		handler_alloc->priority = EPRIO_DEFAULT_REMAP;
	}

	assert(handler_alloc->priority > EPRIO_DEFAULT);
	create_container_at_priority(&global_handlers, handler_alloc->priority, handler_container_prio_func)->data = handler_alloc;

	log_debug("Registered handler: %p %u", *(void**)&handler_alloc->proc, handler_alloc->priority);
	events_print_handlers(global_handlers);
}

void events_unregister_handler(EventHandlerProc proc) {
	ListContainer *next;
	for(ListContainer *c = global_handlers; c; c = next) {
		EventHandler *h = c->data;
		next = c->next;

		if(h->proc == proc) {
			free(c->data);
			delete_element((void**)&global_handlers, c);
			return;
		}
	}
}

static void events_apply_flags(EventFlags flags) {
	if(flags & EFLAG_TEXT) {
		if(!SDL_IsTextInputActive()) {
			SDL_StartTextInput();
		}
	} else {
		if(SDL_IsTextInputActive()) {
			SDL_StopTextInput();
		}
	}

	TaiseiEvent type;

	for(type = TE_MENU_FIRST; type <= TE_MENU_LAST; ++type) {
		SDL_EventState(MAKE_TAISEI_EVENT(type), (bool)(flags & EFLAG_MENU));
	}

	for(type = TE_GAME_FIRST; type <= TE_GAME_LAST; ++type) {
		SDL_EventState(MAKE_TAISEI_EVENT(type), (bool)(flags & EFLAG_GAME));
	}
}

void events_poll(EventHandler *handlers, EventFlags flags) {
	SDL_Event event;
	events_apply_flags(flags);

	while(SDL_PollEvent(&event)) {
		events_invoke_handlers(&event, global_handlers, handlers);
	}
}

void events_emit(TaiseiEvent type, int32_t code, void *data1, void *data2) {
	assert(TAISEI_EVENT_VALID(type));
	uint32_t sdltype = MAKE_TAISEI_EVENT(type);
	assert(IS_TAISEI_EVENT(sdltype));

	if(!SDL_EventState(sdltype, SDL_QUERY)) {
		return;
	}

	SDL_Event event = { 0 };
	event.type = sdltype;
	event.user.code = code;
	event.user.data1 = data1;
	event.user.data2 = data2;

	SDL_PushEvent(&event);
}

void events_pause_keyrepeat(void) {
	// for whatever stupid bizarre reason, keyrepeat skips the delay after the window toggles fullscreen on some systems
	// this ugly hack is a workaround for that
	keyrepeat_paused_until = SDL_GetTicks() + 250;
}

/*
 *	Default handlers
 */

static bool events_handler_quit(SDL_Event *event, void *arg);
static bool events_handler_keyrepeat_workaround(SDL_Event *event, void *arg);
static bool events_handler_clipboard(SDL_Event *event, void *arg);
static bool events_handler_hotkeys(SDL_Event *event, void *arg);
static bool events_handler_key_down(SDL_Event *event, void *arg);
static bool events_handler_key_up(SDL_Event *event, void *arg);


static EventHandler default_handlers[] = {
	{ .proc = events_handler_quit,					.priority = EPRIO_SYSTEM,		.event_type = SDL_QUIT },
	{ .proc = events_handler_keyrepeat_workaround,	.priority = EPRIO_CAPTURE,		.event_type = SDL_KEYDOWN },
	{ .proc = events_handler_clipboard,				.priority = EPRIO_CAPTURE,		.event_type = SDL_KEYDOWN },
	{ .proc = events_handler_hotkeys,				.priority = EPRIO_HOTKEYS,		.event_type = SDL_KEYDOWN },
	{ .proc = events_handler_key_down,				.priority = EPRIO_TRANSLATION,	.event_type = SDL_KEYDOWN },
	{ .proc = events_handler_key_up,				.priority = EPRIO_TRANSLATION,	.event_type = SDL_KEYUP },

	{NULL}
};

static void events_register_default_handlers(void) {
	for(EventHandler *h = default_handlers; h->proc; ++h) {
		events_register_handler(h);
	}
}

static void events_unregister_default_handlers(void) {
	for(EventHandler *h = default_handlers; h->proc; ++h) {
		events_unregister_handler(h->proc);
	}
}

static bool events_handler_quit(SDL_Event *event, void *arg) {
	exit(0);
}

static bool events_handler_keyrepeat_workaround(SDL_Event *event, void *arg) {
	uint32_t timenow;

	if(event->key.repeat && (timenow = SDL_GetTicks()) < keyrepeat_paused_until) {
		log_debug("Prevented a potentially bogus key repeat: %i %i %u",
			event->key.keysym.scancode, event->key.keysym.mod, keyrepeat_paused_until - timenow);
		return true;
	}

	return false;
}

static bool events_handler_clipboard(SDL_Event *event, void *arg) {
	if(!SDL_IsTextInputActive()) {
		return false;
	}

	if(event->key.keysym.mod & KMOD_CTRL && event->key.keysym.scancode == SDL_SCANCODE_V) {
		if(SDL_HasClipboardText()) {
			memset(event, 0, sizeof(SDL_Event));
			event->type = MAKE_TAISEI_EVENT(TE_CLIPBOARD_PASTE);
		} else {
			return true;
		}
	}

	return false;
}

static bool events_handler_key_down(SDL_Event *event, void *arg) {
	SDL_Scancode scan = event->key.keysym.scancode;
	bool repeat = event->key.repeat;

	/*
	 *	Emit menu events
	 */

	struct eventmap_s { int scancode; TaiseiEvent event; } menu_event_map[] = {
		// order matters
		// handle all the hardcoded controls first to prevent accidentally overriding them with unusable ones

		{ SDL_SCANCODE_DOWN, TE_MENU_CURSOR_DOWN },
		{ SDL_SCANCODE_UP, TE_MENU_CURSOR_UP },
		{ SDL_SCANCODE_RIGHT, TE_MENU_CURSOR_RIGHT },
		{ SDL_SCANCODE_LEFT, TE_MENU_CURSOR_LEFT },
		{ SDL_SCANCODE_RETURN, TE_MENU_ACCEPT },
		{ SDL_SCANCODE_ESCAPE, TE_MENU_ABORT },

		{ config_get_int(CONFIG_KEY_DOWN), TE_MENU_CURSOR_DOWN },
		{ config_get_int(CONFIG_KEY_UP), TE_MENU_CURSOR_UP },
		{ config_get_int(CONFIG_KEY_RIGHT), TE_MENU_CURSOR_RIGHT },
		{ config_get_int(CONFIG_KEY_LEFT), TE_MENU_CURSOR_LEFT },
		{ config_get_int(CONFIG_KEY_SHOT), TE_MENU_ACCEPT },
		{ config_get_int(CONFIG_KEY_BOMB), TE_MENU_ABORT },

		{SDL_SCANCODE_UNKNOWN, -1}
	};

	if(!repeat || transition.state == TRANS_IDLE) {
		for(struct eventmap_s *m = menu_event_map; m->scancode != SDL_SCANCODE_UNKNOWN; ++m) {
			if(scan == m->scancode) {
				events_emit(m->event, 0, NULL, NULL);
				break;
			}
		}
	}

	/*
	 *	Emit game events
	 */

	if(!repeat) {
		if(scan == config_get_int(CONFIG_KEY_PAUSE) || scan == SDL_SCANCODE_ESCAPE) {
			events_emit(TE_GAME_PAUSE, 0, NULL, NULL);
		} else {
			int key = config_key_from_scancode(scan);

			if(key >= 0) {
				events_emit(TE_GAME_KEY_DOWN, key, NULL, NULL);
			}
		}
	}

	return false;
}

static bool events_handler_key_up(SDL_Event *event, void *arg) {
	SDL_Scancode scan = event->key.keysym.scancode;

	/*
	 *	Emit game events
	 */

	int key = config_key_from_scancode(scan);

	if(key >= 0) {
		events_emit(TE_GAME_KEY_UP, key, NULL, NULL);
	}

	return false;
}

static bool events_handler_hotkeys(SDL_Event *event, void *arg) {
	if(event->key.repeat) {
		return false;
	}

	SDL_Scancode scan = event->key.keysym.scancode;
	SDL_Keymod mod = event->key.keysym.mod;

	if(scan == config_get_int(CONFIG_KEY_SCREENSHOT)) {
		video_take_screenshot();
		return true;
	}

	if((scan == SDL_SCANCODE_RETURN && (mod & KMOD_ALT)) || scan == config_get_int(CONFIG_KEY_FULLSCREEN)) {
		config_set_int(CONFIG_FULLSCREEN, !config_get_int(CONFIG_FULLSCREEN));
		return true;
	}

	return false;
}
