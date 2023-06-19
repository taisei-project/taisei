/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "events.h"
#include "config.h"
#include "global.h"
#include "video.h"
#include "gamepad.h"

static hrtime_t keyrepeat_paused_until;
static int global_handlers_lock = 0;
static DYNAMIC_ARRAY(EventHandler) global_handlers;
static DYNAMIC_ARRAY(SDL_Event) deferred_events;

uint32_t sdl_first_user_event;

static void events_register_default_handlers(void);
static void events_unregister_default_handlers(void);

#define MAX_ACTIVE_HANDLERS 32

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
	dynarray_free_data(&deferred_events);

#ifdef DEBUG
	dynarray_foreach_elem(&global_handlers, EventHandler *h, {
		log_warn("Global event handler was not unregistered: %p", UNION_CAST(EventHandlerProc, void*, h->proc));
	});
#endif

	dynarray_free_data(&global_handlers);
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

static inline int prio_index(EventPriority prio) {
	return prio - EPRIO_FIRST;
}

void events_register_handler(EventHandler *handler) {
	assert(sdl_first_user_event > 0);
	assert(handler->proc != NULL);
	assert(handler->priority >= EPRIO_FIRST);
	assert(handler->priority <= EPRIO_LAST);

	*dynarray_append(&global_handlers) = *handler;

	// don't bother sorting, since most of the time we will need to re-sort it
	// together with local handlers when polling
}

static bool hfilter_remove_pending(const void *pelem, void *ignored) {
	const EventHandler *h = pelem;
	return !h->_private.removal_pending;
}

void events_unregister_handler(EventHandlerProc proc) {
	dynarray_foreach_elem(&global_handlers, EventHandler *h, {
		if(h->proc == proc && !h->_private.removal_pending) {
			h->_private.removal_pending = true;
			break;
		}
	});

	if(global_handlers_lock) {
		assert(global_handlers_lock > 0);
	} else {
		dynarray_filter(&global_handlers, hfilter_remove_pending, NULL);
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

static int enqueue_event_handlers(int max, EventHandler *queue[max], EventHandler local_handlers[]) {
	// counting sort!

	int cnt = 0;
	int pcount[NUM_EPRIOS] = { 0 };
	EventHandler *temp[max];

	assert(global_handlers.num_elements <= max);

	dynarray_foreach_elem(&global_handlers, EventHandler *h, {
		++pcount[prio_index(h->priority)];
		temp[cnt++] = h;
	});

	if(local_handlers) {
		for(EventHandler *h = local_handlers; h->proc; ++h) {
			int pidx = prio_index(h->priority);
			assert((uint)pidx < ARRAY_SIZE(pcount));
			++pcount[pidx];
			temp[cnt++] = h;
			assert(cnt < max);
		}
	}

	for(int i = 0, total = 0; i < NUM_EPRIOS; ++i) {
		int t = total;
		total += pcount[i];
		pcount[i] = t;
	}

	for(int i = 0; i < cnt; ++i) {
		EventHandler *h = temp[i];
		int p = prio_index(h->priority);
		queue[pcount[p]] = h;
		++pcount[p];
	}

	return cnt;
}

static void push_event(SDL_Event *e) {
	/*
	 * NOTE: The SDL_PushEvent() function is a wrapper around SDL_PeepEvents that also sets the
	 * timestamp field and calls the event filter function and event watchers. We don't use any of
	 * that, and setting the timestamp involves an expensive system call, so avoid it.
	 */
	if(UNLIKELY(SDL_PeepEvents(e, 1, SDL_ADDEVENT, 0, 0) <= 0)) {
		log_sdl_error(LOG_ERROR, "SDL_PeepEvents");
	}
}

void events_poll(EventHandler *handlers, EventFlags flags) {
	events_apply_flags(flags);
	events_emit(TE_FRAME, 0, NULL, NULL);

	++global_handlers_lock;

	EventHandler *hqueue[MAX_ACTIVE_HANDLERS];
	int hqueue_size = enqueue_event_handlers(ARRAY_SIZE(hqueue), hqueue, handlers);

	for(;;) {
		if(!(flags & EFLAG_NOPUMP)) {
			SDL_PumpEvents();
		}

		SDL_Event events[8];
		int nevents = SDL_PeepEvents(events, ARRAY_SIZE(events), SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT);

		if(UNLIKELY(nevents < 0)) {
			log_sdl_error(LOG_ERROR, "SDL_PeepEvents");
		}

		if(nevents == 0) {
			break;
		}

		for(SDL_Event *e = events, *end = events + nevents; e < end; ++e) {
			for(int i = 0; i < hqueue_size; ++i) {
				EventHandler *h = NOT_NULL(hqueue[i]);

				if(h->_private.removal_pending) {
					continue;
				}

				if(events_invoke_handler(e, h)) {
					break;
				}
			}
		}
	}

	if(--global_handlers_lock == 0) {
		dynarray_filter(&global_handlers, hfilter_remove_pending, NULL);
	}

	dynarray_foreach_elem(&deferred_events, SDL_Event *evt, {
		push_event(evt);
	});

	deferred_events.num_elements = 0;
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

	push_event(&event);
}

void events_defer(SDL_Event *evt) {
	*dynarray_append(&deferred_events) = *evt;
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

	if(video_get_backend() == VIDEO_BACKEND_EMSCRIPTEN && scan == SDL_SCANCODE_TAB) {
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

	if(scan == config_get_int(CONFIG_KEY_RELOAD_RESOURCES)) {
		res_reload_all();
		return true;
	}

	return false;
}
