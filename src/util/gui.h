/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#ifndef IGUARD_util_gui_h
#define IGUARD_util_gui_h

#include "taisei.h"

DIAGNOSTIC(push)
DIAGNOSTIC(ignored "-Wstrict-prototypes")
#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui.h>
#include <cimgui_impl.h>
DIAGNOSTIC(pop)

typedef struct GuiPersistentWindow GuiPersistentWindow;
typedef void (*GuiPersistentWindowCallback)(GuiPersistentWindow *w);

struct GuiPersistentWindow {
	const char *name;
	GuiPersistentWindowCallback callback;
	void *userdata;
	ImGuiWindowFlags flags;
	bool open;
	bool manual;
};

void gui_init(void);
void gui_shutdown(void);
void gui_begin_frame(void);
void gui_end_frame(void);
void gui_render(void);
bool gui_wants_text_input(void);
GuiPersistentWindow *gui_add_persistent_window(
	const char *name,
	GuiPersistentWindowCallback callback,
	void *userdata
) attr_nonnull(1, 2);

#endif // IGUARD_util_gui_h
