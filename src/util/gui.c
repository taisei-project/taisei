/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "gui.h"
#include "events.h"
#include "renderer/api.h"

#include <SDL.h>

static bool begun_frame = false;

static bool is_mouse_event(SDL_EventType t) {
	switch(t) {
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEMOTION:
		case SDL_MOUSEWHEEL:
			return true;
		default:
			return false;
	}
}

static bool is_keyboard_event(SDL_EventType t) {
	switch(t) {
		case SDL_KEYUP:
		case SDL_KEYDOWN:
		case SDL_TEXTINPUT:
		case SDL_TEXTEDITING:
			return true;
		default:
			return false;
	}
}

static bool gui_event(SDL_Event *event, void *arg) {
	ImGui_ImplSDL2_ProcessEvent(event);

	ImGuiIO *io = igGetIO();

	if(io->WantCaptureKeyboard && is_keyboard_event(event->type)) {
		return true;
	}

	if(io->WantCaptureMouse && is_mouse_event(event->type)) {
		return true;
	}

	return false;
}

bool gui_wants_text_input(void) {
	ImGuiIO *io = igGetIO();
	return io->WantTextInput;
}

void gui_init(void) {
	igCreateContext(NULL);
	ImGuiIO *io = igGetIO();
	io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
	ImGui_ImplSDL2_InitForOpenGL(
		SDL_GL_GetCurrentWindow(),
		SDL_GL_GetCurrentContext()
	);
	ImGui_ImplOpenGL3_Init("#version 330 core");
	igStyleColorsDark(NULL);
	ImFontAtlas_AddFontDefault(io->Fonts, NULL);
	events_register_handler(&(EventHandler) {
		.proc = gui_event,
		.priority = EPRIO_SYSTEM,
	});
}

void gui_shutdown(void) {
	events_unregister_handler(gui_event);
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	igDestroyContext(NULL);
}

static void display_vfs_path(const char *name, const char *path) {
	assert(path != NULL);

	igTableNextRow(0, 0);
	igTableNextColumn();

	bool is_dir = vfs_query(path).is_dir;

	ImGuiTreeNodeFlags f = ImGuiTreeNodeFlags_SpanFullWidth;

	if(!is_dir) {
		f |=
			ImGuiTreeNodeFlags_Leaf |
			ImGuiTreeNodeFlags_NoTreePushOnOpen;
	}

	bool open = igTreeNodeEx_Str(name, f);
	igTableNextColumn();
	char *repr = vfs_repr(path, false);
	if(repr == NULL) {
		igTextUnformatted("???", NULL);
	} else {
		igTextUnformatted(repr, NULL);
		free(repr);
	}

	igPushID_Str(name);

	if(is_dir && open) {
		VFSDir *vdir = vfs_dir_open(path);
		if(vdir) {
			size_t preflen = strlen(path);
			for(const char *p; (p = vfs_dir_read(vdir));) {
				size_t sublen = strlen(p);
				char fullpath[preflen + sublen + 2];
				memcpy(fullpath, path, preflen);
				fullpath[preflen] = '/';
				memcpy(fullpath+preflen+1, p, sublen);
				fullpath[preflen + 1 + sublen] = 0;
				display_vfs_path(p, fullpath);
			}

			vfs_dir_close(vdir);
		}

		igTreePop();
	}

	igPopID();
}

static void gui_vfs(void) {
	ImGuiWindowFlags f = 0;

	if(!igBegin("VFS view", NULL, f)) {
		igEnd();
		return;
	}

	ImGuiTableFlags treeflags =
		ImGuiTableFlags_BordersV |
		ImGuiTableFlags_BordersOuterH |
		ImGuiTableFlags_Resizable |
		ImGuiTableFlags_RowBg |
		ImGuiTableFlags_NoBordersInBody;

	if(igBeginTable("vfsTreeTable", 2, treeflags, (ImVec2) { 0 }, 0)) {
		igTableSetupColumn("Name", ImGuiTableColumnFlags_NoHide, 0, 0);
		igTableSetupColumn("Type", ImGuiTableColumnFlags_NoHide, 0, 0);
		igTableHeadersRow();

		display_vfs_path("/", "/");

		igEndTable();
	}

	igEnd();
}

void gui_begin_frame(void) {
	assert(!begun_frame);
	begun_frame = true;

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame(SDL_GL_GetCurrentWindow());
	igNewFrame();

	bool showDemo;
	igShowDemoWindow(&showDemo);

	gui_vfs();
}

void gui_end_frame(void) {
	if(begun_frame) {
		begun_frame = false;
		igRender();
	}
}

void gui_render(void) {
	assert(!begun_frame);

	ImDrawData *dd = igGetDrawData();
	if(dd) {
		r_flush_sprites();
		ImGui_ImplOpenGL3_RenderDrawData(dd);
	}
}
