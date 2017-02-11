/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2012, Alexeyew Andrew <http://akari.thebadasschoobs.org/>
 */

#ifndef EVENTS_H
#define EVENTS_H

typedef enum {
	EF_Keyboard 	= 1,
	EF_Text			= 2,
	EF_Menu			= 4,
	EF_Game			= 8,
	EF_Gamepad		= 16
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
	E_GamepadKeyUp
} EventType;

typedef void(*EventHandler)(EventType, int, void*);
void handle_events(EventHandler handler, EventFlags flags, void *arg);

#endif
