/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "gui.h"

#ifndef CIMGUI_STUB
#error Compiling with real cimgui library, your build is broken!
#endif

void gui_init(void) { }
void gui_shutdown(void) { }
void gui_begin_frame(void) { }
void gui_end_frame(void) { }
void gui_render(void) { }
bool gui_wants_text_input(void) { return false; }
GuiPersistentWindow *gui_add_persistent_window(
	const char *name,
	GuiPersistentWindowCallback callback,
	void *userdata
) {
	return NULL;
}
