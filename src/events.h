/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include <SDL3/SDL_events.h>

typedef enum {
	TE_INVALID = -1,

	TE_FRAME,
	TE_RESOURCE_ASYNC_LOADED,
	TE_CONFIG_UPDATED,

	#define TE_MENU_FIRST TE_MENU_CURSOR_UP
	TE_MENU_CURSOR_UP,
	TE_MENU_CURSOR_DOWN,
	TE_MENU_CURSOR_LEFT,
	TE_MENU_CURSOR_RIGHT,
	TE_MENU_ACCEPT,
	TE_MENU_ABORT,
	#define TE_MENU_LAST TE_MENU_ABORT

	#define TE_GAME_FIRST TE_GAME_KEY_DOWN
	TE_GAME_KEY_DOWN,
	TE_GAME_KEY_UP,
	TE_GAME_AXIS_UD,
	TE_GAME_AXIS_LR,
	TE_GAME_PAUSE,
	#define TE_GAME_LAST TE_GAME_PAUSE

	TE_GAME_PAUSE_STATE_CHANGED,

	TE_GAMEPAD_BUTTON_DOWN,
	TE_GAMEPAD_BUTTON_UP,
	TE_GAMEPAD_AXIS,
	TE_GAMEPAD_AXIS_DIGITAL,

	TE_VIDEO_MODE_CHANGED,

	TE_CLIPBOARD_PASTE,

	TE_AUDIO_BGM_STARTED,

	TE_FILEWATCH,

	TE_WATCHDOG,

	NUM_TAISEI_EVENTS
} TaiseiEvent;

typedef enum {
	// from highest to lowest
	// feel free to add new prios as needed, just don't randomly reorder stuff

	EPRIO_SYSTEM = -4,  // for events not associated with user input
	EPRIO_TRANSLATION,  // for translating raw input events into higher level Taisei events
	EPRIO_CAPTURE,      // for capturing raw user input before it's further processed
	EPRIO_HOTKEYS,      // for global keybindings
	EPRIO_NORMAL,       // for everything else

	EPRIO_DEFAULT = EPRIO_NORMAL,
	EPRIO_FIRST = EPRIO_SYSTEM,
	EPRIO_LAST = EPRIO_NORMAL,
	NUM_EPRIOS = EPRIO_LAST - EPRIO_FIRST + 1,
} EventPriority;

static_assert(EPRIO_DEFAULT == 0);

typedef enum {
	EFLAG_MENU = (1 << 0),
	EFLAG_GAME = (1 << 1),
	EFLAG_TEXT = (1 << 2),
	EFLAG_NOPUMP = (1 << 3),
} EventFlags;

typedef enum {
	INDEV_KEYBOARD,
	INDEV_GAMEPAD,
} InputDevice;

// if the this returns true, the event won't be passed down to lower priority handlers
typedef bool (*EventHandlerProc)(SDL_Event *event, void *arg);

typedef struct EventHandler {
	EventHandlerProc proc;
	void *arg;
	EventPriority priority;
	uint32_t event_type; // if 0, this handler gets all events

	struct {
		bool removal_pending;
	} _private;
} EventHandler;

extern uint32_t sdl_first_user_event;

// Convert a TaiseiEvent code to an SDL event type
#define MAKE_TAISEI_EVENT(te_code) (sdl_first_user_event + (te_code))

// Convert an SDL event type to a TaiseiEvent code
#define TAISEI_EVENT(sdl_etype) ((sdl_etype) - sdl_first_user_event)

// Check if argument is a valid TaiseiEvent code
#define TAISEI_EVENT_VALID(te_code) ((unsigned)(te_code) < NUM_TAISEI_EVENTS)

// Check if an SDL event type is a Taisei event
#define IS_TAISEI_EVENT(sdl_etype) (TAISEI_EVENT_VALID(TAISEI_EVENT(sdl_etype)))

void events_init(void);
void events_shutdown(void);
void events_pause_keyrepeat(void);
void events_register_handler(EventHandler *handler);
void events_unregister_handler(EventHandlerProc proc);
void events_poll(EventHandler *handlers, EventFlags flags);
void events_emit(TaiseiEvent type, int32_t code, void *data1, void *data2);
void events_defer(SDL_Event *evt);
