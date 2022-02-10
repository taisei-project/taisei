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
#include "resource/model.h"

#ifdef CIMGUI_STUB
#error Compiling with stub cimgui library, your build is broken!
#endif

#include <SDL.h>

#define MAX_VERTICES (1 << 16)

typedef DYNAMIC_ARRAY(GuiPersistentWindow) GuiPersistentWindowArray;

static struct {
	VertexArray *va;
	VertexBuffer *vb;
	IndexBuffer *ib;
	ShaderProgram *shader;
	Uniform *texture_uniform;
	Texture *atlas;
	GuiPersistentWindowArray persistent_windows;
	bool begun_frame;
} gui;

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

static void gui_update_atlas(void);

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

static void gui_post_init(void);

void gui_init(void) {
	NOT_NULL(igCreateContext(NULL));

	preload_resources(RES_SHADER_PROGRAM, RESF_DEFAULT | RESF_PERMANENT, "imgui", NULL);
	SDL_EventState(SDL_MOUSEMOTION, SDL_ENABLE);

	ImGuiIO *io = igGetIO();
	io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
// 	io->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
	io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io->ConfigFlags |= ImGuiConfigFlags_NavNoCaptureKeyboard;
	ImGui_ImplSDL2_InitForOpenGL(
		SDL_GL_GetCurrentWindow(),
		SDL_GL_GetCurrentContext()
	);
	igStyleColorsDark(NULL);
	ImFontAtlas_AddFontDefault(io->Fonts, NULL);
	events_register_handler(&(EventHandler) {
		.proc = gui_event,
		.priority = EPRIO_SYSTEM,
	});

	size_t sz_vert = sizeof(ImDrawVert);
	#define VERTEX_OFS(attr) offsetof(ImDrawVert, attr)

	VertexAttribFormat fmt[] = {
		{ { 2, VA_FLOAT, VA_CONVERT_FLOAT,            0 }, sz_vert, VERTEX_OFS(pos),       0 },
		{ { 2, VA_FLOAT, VA_CONVERT_FLOAT,            0 }, sz_vert, VERTEX_OFS(uv),        0 },
		{ { 4, VA_UBYTE, VA_CONVERT_FLOAT_NORMALIZED, 0 }, sz_vert, VERTEX_OFS(col),       0 },
	};

	#undef VERTEX_OFS

	gui.vb = r_vertex_buffer_create(sz_vert * MAX_VERTICES, NULL);
	r_vertex_buffer_set_debug_label(gui.vb, "ImGui VB");

	gui.ib = r_index_buffer_create(sizeof(ImDrawIdx), MAX_VERTICES);
	r_index_buffer_set_debug_label(gui.ib, "ImGui IB");

	gui.va = r_vertex_array_create();
	r_vertex_array_set_debug_label(gui.va, "ImGui VA");
	r_vertex_array_layout(gui.va, ARRAY_SIZE(fmt), fmt);
	r_vertex_array_attach_vertex_buffer(gui.va, gui.vb, 0);
	r_vertex_array_attach_index_buffer(gui.va, gui.ib);

	gui.shader = res_shader("imgui");
	gui.texture_uniform = r_shader_uniform(gui.shader, "tex");

	gui_update_atlas();

	io->BackendRendererUserData = &gui;
	io->BackendRendererName = "taiseiapi";

	gui_post_init();
}

static void gui_update_atlas(void) {
	if(gui.atlas) {
		r_texture_destroy(gui.atlas);
	}

	ImGuiIO *io = igGetIO();

	Pixmap px = { 0 };
	px.origin = PIXMAP_ORIGIN_TOPLEFT;
	px.format = PIXMAP_FORMAT_R8;

	uint8_t *src;
	ImFontAtlas_GetTexDataAsAlpha8(
		io->Fonts,
		&src,
		(int*)&px.width,
		(int*)&px.height,
		NULL
	);

	px.data.untyped = pixmap_alloc_buffer_for_copy(&px, &px.data_size);
	memcpy(px.data.untyped, src, px.data_size);

	TextureParams p = { 0 };
	p.width = px.width;
	p.height = px.height;
	p.filter.min = TEX_FILTER_LINEAR;
	p.filter.mag = TEX_FILTER_LINEAR;

	SwizzleMask sw = { "rrrr" };

	if(r_supports(RFEAT_TEXTURE_SWIZZLE)) {
		p.type = TEX_TYPE_R_8;
		p.swizzle = sw;
	} else {
		p.type = TEX_TYPE_RGBA_8;
		pixmap_convert_inplace_realloc(&px, PIXMAP_FORMAT_RGBA8);
		pixmap_swizzle_inplace(&px, sw);
	}

	TextureTypeQueryResult qr;
	if(!r_texture_type_query(p.type, p.flags, px.format, px.origin, &qr)) {
		UNREACHABLE;
	}

	pixmap_convert_inplace_realloc(&px, qr.optimal_pixmap_format);
	pixmap_flip_to_origin_inplace(&px, qr.optimal_pixmap_origin);

	gui.atlas = r_texture_create(&p);
	r_texture_set_debug_label(gui.atlas, "ImGui atlas");
	r_texture_fill(gui.atlas, 0, 0, &px);
	free(px.data.untyped);

	ImFontAtlas_SetTexID(io->Fonts, (ImTextureID)gui.atlas);
}

void gui_shutdown(void) {
	r_texture_destroy(gui.atlas);
	r_vertex_array_destroy(gui.va);
	r_vertex_buffer_destroy(gui.vb);
	r_index_buffer_destroy(gui.ib);
	events_unregister_handler(gui_event);
	ImGui_ImplSDL2_Shutdown();
	igDestroyContext(NULL);
}

GuiPersistentWindow *gui_add_persistent_window(const char *name, GuiPersistentWindowCallback callback, void *userdata) {
	GuiPersistentWindow *w = dynarray_append(&gui.persistent_windows);
	w->name = name;
	w->callback = callback;
	w->userdata = userdata;
	return w;
}

static void gui_show_persistent(void) {
	dynarray_foreach_elem(&gui.persistent_windows, GuiPersistentWindow *w, {
		if(!w->open) {
			continue;
		}

		if(w->manual) {
			w->callback(w);
			continue;
		}

		if(!igBegin(w->name, &w->open, w->flags)) {
			igEnd();
			continue;
		}

		w->callback(w);

		igEnd();
	});
}

void gui_begin_frame(void) {
	assert(!gui.begun_frame);
	gui.begun_frame = true;

	ImGui_ImplSDL2_NewFrame(SDL_GL_GetCurrentWindow());
	igNewFrame();

	ImGuiDockNodeFlags f = ImGuiDockNodeFlags_PassthruCentralNode;
	igDockSpaceOverViewport(igGetMainViewport(), f, NULL);

	if(igBeginPopupContextVoid(NULL, ImGuiPopupFlags_MouseButtonRight)) {
		dynarray_foreach_elem(&gui.persistent_windows, GuiPersistentWindow *w, {
			igMenuItem_BoolPtr(w->name, NULL, &w->open, true);
		});
		igEndPopup();
	}

	gui_show_persistent();
}

void gui_end_frame(void) {
	if(gui.begun_frame) {
		gui.begun_frame = false;
		igRender();
	}
}

void gui_render(void) {
	assert(!gui.begun_frame);

	ImDrawData *dd = igGetDrawData();

	if(!dd) {
		return;
	}

	int fb_width = (int)(dd->DisplaySize.x * dd->FramebufferScale.x);
	int fb_height = (int)(dd->DisplaySize.y * dd->FramebufferScale.y);

	if(fb_width <= 0 || fb_height <= 0) {
		return;
	}

	r_flush_sprites();

	Framebuffer *fb = r_framebuffer_current();
	FloatRect fb_prev_viewport;
	r_framebuffer_viewport_current(fb, &fb_prev_viewport);
	r_framebuffer_viewport(fb, 0, 0, fb_width, fb_height);

	ImVec2 clip_off = dd->DisplayPos;
	ImVec2 clip_scale = dd->FramebufferScale;

	r_flush_sprites();
	r_state_push();
	r_blend(BLEND_PREMUL_ALPHA);
	r_disable(RCAP_CULL_FACE);
	r_disable(RCAP_DEPTH_TEST);
	r_shader_ptr(gui.shader);

	r_mat_proj_push_ortho_ex(
		dd->DisplayPos.x,
		dd->DisplayPos.x + dd->DisplaySize.x,
		dd->DisplayPos.y + dd->DisplaySize.y,
		dd->DisplayPos.y,
		-1, 1
	);

	Model mdl = { 0 };
	mdl.primitive = PRIM_TRIANGLES;
	mdl.vertex_array = gui.va;

	SDL_RWops *vstream = r_vertex_buffer_get_stream(gui.vb);

	for(int i = 0; i < dd->CmdListsCount; ++i) {
		ImDrawList *clist = dd->CmdLists[i];

		SDL_RWwrite(vstream, clist->VtxBuffer.Data, clist->VtxBuffer.Size * sizeof(ImDrawVert), 1);
		_Generic(clist->IdxBuffer.Data,
			uint16_t*: r_index_buffer_add_indices_u16,
			uint32_t*: r_index_buffer_add_indices_u32
		)(gui.ib, 0, clist->IdxBuffer.Size, clist->IdxBuffer.Data);

		for(int c = 0; c < clist->CmdBuffer.Size; ++c) {
			ImDrawCmd *cmd = clist->CmdBuffer.Data + c;

			if(cmd->UserCallback) {
				// TODO handle ImDrawCallback_ResetRenderState?
				cmd->UserCallback(clist, cmd);
				continue;
			}

			ImVec4 clip_rect;
			clip_rect.x = (cmd->ClipRect.x - clip_off.x) * clip_scale.x;
			clip_rect.y = (cmd->ClipRect.y - clip_off.y) * clip_scale.y;
			clip_rect.z = (cmd->ClipRect.z - clip_off.x) * clip_scale.x;
			clip_rect.w = (cmd->ClipRect.w - clip_off.y) * clip_scale.y;

			if(
				clip_rect.x < fb_width &&
				clip_rect.y < fb_height &&
				clip_rect.z >= 0.0f &&
				clip_rect.w >= 0.0f
			) {
				r_scissor(
					(int)clip_rect.x,
					(int)clip_rect.y,
					(int)(clip_rect.z - clip_rect.x),
					(int)(clip_rect.w - clip_rect.y)
				);

				r_uniform_sampler(gui.texture_uniform, (Texture*)ImDrawCmd_GetTexID(cmd));
				mdl.num_indices = cmd->ElemCount;
				mdl.offset = cmd->IdxOffset;
				r_draw_model_ptr(&mdl, 0, 0);
			}
		}

		r_vertex_buffer_invalidate(gui.vb);
		r_index_buffer_invalidate(gui.ib);
	}

	r_framebuffer_viewport_rect(fb, fb_prev_viewport);
	r_mat_proj_pop();
	r_state_pop();
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

static void gui_window_vfs(GuiPersistentWindow *w) {
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
}

static void gui_window_demo(GuiPersistentWindow *w) {
	igShowDemoWindow(&w->open);
}

static void gui_post_init(void) {
	GuiPersistentWindow *w;
	gui_add_persistent_window("VFS view", gui_window_vfs, NULL);

	w = gui_add_persistent_window("ImGui demo", gui_window_demo, NULL);
	w->manual = true;
}
