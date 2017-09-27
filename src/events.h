/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once

#include "config.h"

typedef enum {
	EF_Keyboard 	= 1,
	EF_Text			= 2,
	EF_Menu			= 4,
	EF_Game			= 8,
	EF_Gamepad		= 16,
	EF_Video		= 32,
} EventFlags;

typedef enum {
	// EF_Keyboard
	E_KeyDown,
	E_KeyUp,

	// EF_Text
	E_CharTyped,
	E_CharErased,
	E_SubmitText,
	E_CancelText,

	// EF_Menu
	E_CursorUp,
	E_CursorDown,
	E_CursorLeft,
	E_CursorRight,
	E_MenuAccept,
	E_MenuAbort,

	// EF_Game
	E_PlrKeyDown,
	E_PlrKeyUp,
	E_PlrAxisUD,
	E_PlrAxisLR,
	E_Pause,

	// EF_Gamepad
	E_GamepadKeyDown,
	E_GamepadKeyUp,
	E_GamepadAxis,
	E_GamepadAxisValue,

	// EF_Video
	E_VideoModeChanged,
} EventType;

extern struct sdl_custom_events_s {
	uint32_t resource_load_finished;
	uint32_t video_mode_changed;
} sdl_custom_events;

#define NUM_CUSTOM_EVENTS (sizeof(struct sdl_custom_events_s)/sizeof(uint32_t))
#define FIRST_CUSTOM_EVENT (((uint32_t*)&sdl_custom_events)[0])
#define LAST_CUSTOM_EVENT  (((uint32_t*)&sdl_custom_events)[NUM_CUSTOM_EVENTS - 1])
#define IS_CUSTOM_EVENT(e) ((e) >= FIRST_CUSTOM_EVENT && (e) <= LAST_CUSTOM_EVENT)

typedef void(*EventHandler)(EventType, int, void*);

void events_init(void);
void events_pause_keyrepeat(void);
void handle_events(EventHandler handler, EventFlags flags, void *arg);
bool gamekeypressed(KeyIndex key);


