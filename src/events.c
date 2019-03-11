/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "events.h"
#include "config.h"
#include "global.h"
#include "video.h"
#include "gamepad.h"

typedef struct EventHandlerContainer {
	LIST_INTERFACE(struct EventHandlerContainer);
	EventHandler *handler;
} EventHandlerContainer;

typedef LIST_ANCHOR(EventHandlerContainer) EventHandlerList;

static hrtime_t keyrepeat_paused_until;
static EventHandlerList global_handlers;
static int global_handlers_lock = 0;

uint32_t sdl_first_user_event;

static void events_register_default_handlers(void);
static void events_unregister_default_handlers(void);

/*
 *  Public API
 */

void events_init(void) {
	sdl_first_user_event = SDL_RegisterEvents(NUM_TAISEI_EVENTS);

	if(sdl_first_user_event == (uint32_t)-1) {
		char *s =
			"You have exhausted the SDL userevent pool. "
			"How you managed that is beyond me, but congratulations.";
		log_fatal("%s", s);
	}

	SDL_EventState(SDL_MOUSEMOTION, SDL_DISABLE);

	events_register_default_handlers();
}

void events_shutdown(void) {
	events_unregister_default_handlers();

#ifdef DEBUG
	if(global_handlers.first) {
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
	assert(global_handlers_lock > 0);

	if(!handler->event_type || handler->event_type == event->type) {
		bool result = handler->proc(event, handler->arg);
		return result;
	}

	return false;
}

static int handler_container_prio_func(List *h) {
	return ((EventHandlerContainer*)h)->handler->priority;
}

static EventPriority real_priority(EventPriority prio) {
	if(prio == EPRIO_DEFAULT) {
		return EPRIO_DEFAULT_REMAP;
	}

	assert(prio < NUM_EPRIOS);
	return prio;
}

static EventHandlerContainer* ehandler_wrap_container(EventHandler *handler) {
	EventHandlerContainer *c = calloc(1, sizeof(*c));
	c->handler = handler;
	return c;
}

static int ehandler_sort_cmp(const void *a, const void *b) {
	EventHandler *const *h0 = a;
	EventHandler *const *h1 = b;

	EventPriority p0 = real_priority((*h0)->priority);
	EventPriority p1 = real_priority((*h1)->priority);

	int diff = (p0 > p1) - (p1 > p0);

	if(diff == 0) {
		diff = ((intptr_t)h0 > (intptr_t)h1) - ((intptr_t)h1 > (intptr_t)h0);
	}

	return diff;
}

static bool _events_invoke_handlers(SDL_Event *event, EventHandlerContainer *h_list, EventHandler *h_array) {
	// invoke handlers from two sources (a list and an array) in the correct order according to priority
	// list items take precedence
	//
	// assumptions:
	//      h_list is sorted by priority
	//      h_array is in arbitrary order and terminated with a NULL-proc entry

	bool result = false;

	if(h_list && !h_array) {
		// case 1 (simplest): we have a list and no custom handlers

		for(EventHandlerContainer *c = h_list; c; c = c->next) {
			if(c->handler->_private.removal_pending) {
				continue;
			}

			if((result = events_invoke_handler(event, c->handler))) {
				break;
			}
		}

		return result;
	}

	if(h_list && h_array) {
		// case 2 (suboptimal): we have both a list and a disordered array; need to do some actual work
		// if you want to optimize this be my guest
		uint cnt = 0, idx =0;

		for(EventHandlerContainer *c = h_list; c; c = c->next) {
			++cnt;
		}

		for(EventHandler *h = h_array; h->proc; ++h) {
			++cnt;
		}

		EventHandler *merged[cnt];

		for(EventHandlerContainer *c = h_list; c; c = c->next, ++idx) {
			merged[idx] = c->handler;
		}

		for(EventHandler *h = h_array; h->proc; ++h, ++idx) {
			merged[idx] = h;
		}

		qsort(merged, cnt, sizeof(*merged), ehandler_sort_cmp);

		for(int i = 0; i < cnt; ++i) {
			EventHandler *e = merged[i];

			if(e->_private.removal_pending) {
				continue;
			}

			if((result = events_invoke_handler(event, e))) {
				break;
			}
		}
	}

	if(!h_list && h_array) {
		// case 3 (unlikely): we don't have a list for some reason (no global handlers?), but there are custom handlers

		for(EventHandler *h = h_array; h->proc; ++h) {
			if((result = events_invoke_handler(event, h))) {
				break;
			}
		}

		return result;
	}

	// case 4 (unlikely): huh? okay then
	return result;
}

static bool events_invoke_handlers(SDL_Event *event, EventHandlerContainer *h_list, EventHandler *h_array) {
	++global_handlers_lock;
	bool result = _events_invoke_handlers(event, h_list, h_array);

	if(--global_handlers_lock == 0) {
		for(EventHandlerContainer *c = global_handlers.first, *next; c; c = next) {
			next = c->next;

			if(c->handler->_private.removal_pending) {
				free(c->handler);
				free(alist_unlink(&global_handlers, c));
			}
		}
	}

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
	alist_insert_at_priority_tail(
		&global_handlers,
		ehandler_wrap_container(handler_alloc),
		handler_alloc->priority,
		handler_container_prio_func
	);
}

void events_unregister_handler(EventHandlerProc proc) {
	for(EventHandlerContainer *c = global_handlers.first, *next; c; c = next) {
		EventHandler *h = c->handler;
		next = c->next;

		if(h->proc == proc) {
			if(global_handlers_lock) {
				assert(global_handlers_lock > 0);
				c->handler->_private.removal_pending = true;
				return;
			}

			free(c->handler);
			free(alist_unlink(&global_handlers, c));
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
	events_emit(TE_FRAME, 0, NULL, NULL);

	while(SDL_PollEvent(&event)) {
		events_invoke_handlers(&event, global_handlers.first, handlers);
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
	// workaround for SDL bug
	// https://bugzilla.libsdl.org/show_bug.cgi?id=3287
	keyrepeat_paused_until = time_get() + HRTIME_RESOLUTION / 4;
}

/*
 *  Default handlers
 */

static bool events_handler_quit(SDL_Event *event, void *arg);
static bool events_handler_config(SDL_Event *event, void *arg);
static bool events_handler_keyrepeat_workaround(SDL_Event *event, void *arg);
static bool events_handler_clipboard(SDL_Event *event, void *arg);
static bool events_handler_hotkeys(SDL_Event *event, void *arg);
static bool events_handler_key_down(SDL_Event *event, void *arg);
static bool events_handler_key_up(SDL_Event *event, void *arg);

attr_unused
static bool events_handler_debug_winevt(SDL_Event *event, void *arg) {
	// copied from SDL wiki almost verbatim

	if(event->type == SDL_WINDOWEVENT) {
		switch(event->window.event) {
			case SDL_WINDOWEVENT_SHOWN:
				log_info("Window %d shown", event->window.windowID);
				break;
			case SDL_WINDOWEVENT_HIDDEN:
				log_info("Window %d hidden", event->window.windowID);
				break;
			case SDL_WINDOWEVENT_EXPOSED:
				log_info("Window %d exposed", event->window.windowID);
				break;
			case SDL_WINDOWEVENT_MOVED:
				log_info("Window %d moved to %d,%d", event->window.windowID, event->window.data1, event->window.data2);
				break;
			case SDL_WINDOWEVENT_RESIZED:
				log_info("Window %d resized to %dx%d", event->window.windowID, event->window.data1, event->window.data2);
				break;
			case SDL_WINDOWEVENT_SIZE_CHANGED:
				log_info("Window %d size changed to %dx%d", event->window.windowID, event->window.data1, event->window.data2);
				break;
			case SDL_WINDOWEVENT_MINIMIZED:
				log_info("Window %d minimized", event->window.windowID);
				break;
			case SDL_WINDOWEVENT_MAXIMIZED:
				log_info("Window %d maximized", event->window.windowID);
				break;
			case SDL_WINDOWEVENT_RESTORED:
				log_info("Window %d restored", event->window.windowID);
				break;
			case SDL_WINDOWEVENT_ENTER:
				log_info("Mouse entered window %d", event->window.windowID);
				break;
			case SDL_WINDOWEVENT_LEAVE:
				log_info("Mouse left window %d", event->window.windowID);
				break;
			case SDL_WINDOWEVENT_FOCUS_GAINED:
				log_info("Window %d gained keyboard focus", event->window.windowID);
				break;
			case SDL_WINDOWEVENT_FOCUS_LOST:
				log_info("Window %d lost keyboard focus", event->window.windowID);
				break;
			case SDL_WINDOWEVENT_CLOSE:
				log_info("Window %d closed", event->window.windowID);
				break;
			case SDL_WINDOWEVENT_TAKE_FOCUS:
				log_info("Window %d is offered a focus", event->window.windowID);
				break;
			case SDL_WINDOWEVENT_HIT_TEST:
				log_info("Window %d has a special hit test", event->window.windowID);
				break;
			default:
				log_warn("Window %d got unknown event %d", event->window.windowID, event->window.event);
			break;
		}
	}

	return false;
}


static EventHandler default_handlers[] = {
#ifdef DEBUG_WINDOW_EVENTS
	{ .proc = events_handler_debug_winevt,          .priority = EPRIO_SYSTEM,       .event_type = SDL_WINDOWEVENT },
#endif
	{ .proc = events_handler_quit,                  .priority = EPRIO_SYSTEM,       .event_type = SDL_QUIT },
	{ .proc = events_handler_config,                .priority = EPRIO_SYSTEM,       .event_type = 0 },
	{ .proc = events_handler_keyrepeat_workaround,  .priority = EPRIO_CAPTURE,      .event_type = 0 },
	{ .proc = events_handler_clipboard,             .priority = EPRIO_CAPTURE,      .event_type = SDL_KEYDOWN },
	{ .proc = events_handler_hotkeys,               .priority = EPRIO_HOTKEYS,      .event_type = SDL_KEYDOWN },
	{ .proc = events_handler_key_down,              .priority = EPRIO_TRANSLATION,  .event_type = SDL_KEYDOWN },
	{ .proc = events_handler_key_up,                .priority = EPRIO_TRANSLATION,  .event_type = SDL_KEYUP },

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
	taisei_quit();
	return true;
}

static bool events_handler_config(SDL_Event *event, void *arg) {
	if(event->type != MAKE_TAISEI_EVENT(TE_CONFIG_UPDATED)) {
		return false;
	}

	ConfigValue *val = event->user.data1;

	switch(event->user.code) {
		case CONFIG_GAMEPAD_ENABLED:
			// TODO: Refactor gamepad code so that we don't have to do this.
			if(val->i) {
				gamepad_init();
			} else {
				gamepad_shutdown();
			}
			break;
	}

	return false;
}

static bool events_handler_keyrepeat_workaround(SDL_Event *event, void *arg) {
	hrtime_t timenow = time_get();

	if(event->type != SDL_KEYDOWN) {
		uint32_t te = TAISEI_EVENT(event->type);

		if(te < TE_MENU_FIRST || te > TE_MENU_LAST) {
			return false;
		}
	}

	if(timenow < keyrepeat_paused_until) {
		log_debug(
			"Prevented a potentially bogus key repeat (%"PRIuTIME" remaining). "
			"This is an SDL bug. See https://bugzilla.libsdl.org/show_bug.cgi?id=3287",
			keyrepeat_paused_until - timenow
		);
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

	if(video.backend == VIDEO_BACKEND_EMSCRIPTEN && scan == SDL_SCANCODE_TAB) {
		scan = SDL_SCANCODE_ESCAPE;
	}

	/*
	 *  Emit menu events
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
			if(scan == m->scancode && (!repeat || m->event != TE_MENU_ACCEPT)) {
				events_emit(m->event, 0, (void*)(intptr_t)INDEV_KEYBOARD, NULL);
				break;
			}
		}
	}

	/*
	 *  Emit game events
	 */

	if(!repeat) {
		if(scan == config_get_int(CONFIG_KEY_PAUSE) || scan == SDL_SCANCODE_ESCAPE) {
			events_emit(TE_GAME_PAUSE, 0, NULL, NULL);
		} else {
			int key = config_key_from_scancode(scan);

			if(key >= 0) {
				events_emit(TE_GAME_KEY_DOWN, key, (void*)(intptr_t)INDEV_KEYBOARD, NULL);
			}
		}
	}

	return false;
}

static bool events_handler_key_up(SDL_Event *event, void *arg) {
	SDL_Scancode scan = event->key.keysym.scancode;

	/*
	 *  Emit game events
	 */

	int key = config_key_from_scancode(scan);

	if(key >= 0) {
		events_emit(TE_GAME_KEY_UP, key, (void*)(intptr_t)INDEV_KEYBOARD, NULL);
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
		video_set_fullscreen(!video_is_fullscreen());
		return true;
	}

	if(scan == config_get_int(CONFIG_KEY_TOGGLE_AUDIO)) {
		config_set_int(CONFIG_MUTE_AUDIO, !config_get_int(CONFIG_MUTE_AUDIO));
		return true;
	}

	return false;
}
