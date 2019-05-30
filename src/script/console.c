/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "console.h"
#include "events.h"
#include "resource/font.h"
#include "video.h"
#include "util.h"
#include "util/graphics.h"
#include "version.h"
#include "script.h"
#include "repl.h"

#define CON_MAXLINES 1024
#define CON_MAXHISTORY 64
#define CON_LINELEN 512
#define CON_FONT "monotiny"
#define CON_LINE_PREFIX "] "

static struct {
	struct {
		ResourceRef font;
		ResourceRef shader;
	} res;

	struct {
		char *buffer;
		struct {
			uint index;
			uint cursor;
		} current;
		uint num;
		uint scroll_pos;
	} lines;

	struct {
		uint32_t *buffer;
		uint cursor;
		uint prompt_len;
	} input;

	struct {
		char *buffer;
		char *input_saved;
		uint current;
		int selected;
		uint num;
	} history;

	uint frames;
	bool active;
} console;

static bool con_event(SDL_Event *evt, void *arg);

#define LINE_POINTER(idx, ofs) (console.lines.buffer + (idx) * CON_LINELEN + (ofs))
#define INPUT_POINTER(ofs) (console.input.buffer + (ofs))
#define HISTORY_POINTER(idx) (console.history.buffer + (idx) * CON_LINELEN)

static void con_print_internal(const char *text, const char *line_prefix) {
	char *line = LINE_POINTER(console.lines.current.index, console.lines.current.cursor);
	char *line_end = line + CON_LINELEN - 1;
	uint pref_len = strlen(line_prefix);

	for(const char *c = text; *c; ++c) {
		if(*c == '\n' || line == line_end) {
			*line = 0;
			log_info("%s", LINE_POINTER(console.lines.current.index, 0));
			console.lines.current.index = (console.lines.current.index + 1) % CON_MAXLINES;

			if(console.lines.num <= console.lines.current.index) {
				console.lines.buffer = realloc(console.lines.buffer, ++console.lines.num * CON_LINELEN);
			}

			line = LINE_POINTER(console.lines.current.index, 0);
			line_end = line + CON_LINELEN - 1;

			for(const char *p = line_prefix; *p; ++p) {
				*line++ = *p;
			}

			console.lines.current.cursor = pref_len;

			if(*c != '\n') {
				*line++ = *c;
				++console.lines.current.cursor;
			}
		} else if(*c == '\r') {
			console.lines.current.cursor = pref_len;
			line = LINE_POINTER(console.lines.current.index, console.lines.current.cursor);
		} else if(*c == '\b') {
			console.lines.current.cursor = imax(pref_len, (int)console.lines.current.cursor - 1);
			line = LINE_POINTER(console.lines.current.index, console.lines.current.cursor);
		} else {
			*line++ = *c;
			++console.lines.current.cursor;
		}
	}
}

void con_print(const char *text) {
	con_print_internal(text, CON_LINE_PREFIX);
}

void con_printf(const char *fmt, ...) {
	char buf[CON_LINELEN];
	va_list va;
	va_start(va, fmt);
	vsnprintf(buf, sizeof(buf), fmt, va);
	va_end(va);
	con_print(buf);
}

static void con_insert(const char *text_utf8) {
	if(console.input.cursor >= CON_LINELEN) {
		return;
	}

	uint32_t *input_ptr = INPUT_POINTER(console.input.cursor);
	// uint32_t *input_end = INPUT_POINTER(CON_LINELEN - 1);
	size_t tail_len = ucs4len(input_ptr);
	uint32_t tail[tail_len + 1];
	memcpy(tail, input_ptr, sizeof(tail));

	assert(tail_len + console.input.cursor == ucs4len(INPUT_POINTER(0)));
	uint space_left = CON_LINELEN - console.input.cursor - tail_len - 1;

	while(*text_utf8 && space_left) {
		uint32_t chr = utf8_getch(&text_utf8);
		*input_ptr++ = chr;
		--space_left;
		console.input.cursor++;
	}

	memcpy(input_ptr, tail, sizeof(tail));
	console.lines.scroll_pos = 0;
}

static void con_erase(int num) {
	if(num < 0) {
		int dest_ofs = (int)console.input.cursor + num;

		if(dest_ofs < console.input.prompt_len) {
			dest_ofs = console.input.prompt_len;
		}

		if(console.input.cursor == dest_ofs) {
			return;
		}

		uint32_t *input_ptr = INPUT_POINTER(console.input.cursor);
		memmove(console.input.buffer + dest_ofs, input_ptr, (ucs4len(input_ptr) + 1) * sizeof(*console.input.buffer));
		console.input.cursor = dest_ofs;
	} else if(num > 0) {
		uint32_t *input_ptr = INPUT_POINTER(console.input.cursor);
		size_t tail_len = ucs4len(input_ptr);

		if(num > tail_len) {
			num = tail_len;
		}

		memmove(input_ptr, input_ptr + num, (tail_len - num + 1) * sizeof(*console.input.buffer));
	}

	console.lines.scroll_pos = 0;
}

static inline const char* con_fetch_history(int ofs) {
	return HISTORY_POINTER((console.history.current - ofs - 1) % CON_MAXHISTORY);
}

static void con_add_history(char *line) {
	if(!*line || (console.history.num > 0 && !strcmp(line, con_fetch_history(0)))) {
		return;
	}

	if(console.history.num <= console.history.current) {
		console.history.num++;
		console.history.buffer = realloc(console.history.buffer, CON_MAXHISTORY * CON_LINELEN);
	}

	memcpy(HISTORY_POINTER(console.history.current), line, strlen(line) + 1);
	console.history.current = (console.history.current + 1) % CON_MAXHISTORY;
}

static void con_pull_history(int ofs) {
	ofs = iclamp(ofs, -1, (int)console.history.num - 1);

	if(ofs == console.history.selected) {
		return;
	}

	uint32_t *ip = INPUT_POINTER(console.input.prompt_len);

	if(ofs == -1) {
		assert(console.history.input_saved != NULL);
		utf8_to_ucs4(console.history.input_saved, CON_LINELEN - console.input.prompt_len, ip);
		free(console.history.input_saved);
		console.history.input_saved = NULL;
	} else {
		if(console.history.input_saved == NULL) {
			console.history.input_saved = ucs4_to_utf8_alloc(ip);
		}

		const char *histline = con_fetch_history(ofs);
		utf8_to_ucs4(histline, CON_LINELEN - console.input.prompt_len, ip);
	}

	console.input.cursor = ucs4len(ip) + console.input.prompt_len;
	console.history.selected = ofs;
}

static inline void con_cycle_history(int ofs) {
	con_pull_history(console.history.selected + ofs);
	console.lines.scroll_pos = 0;
}

static void con_submit(void) {
	uint32_t *input = INPUT_POINTER(0);
	char *utf8line = ucs4_to_utf8_alloc(input);
	con_print_internal("\r", "");
	con_print_internal(utf8line, "");
	con_print("\n");
	con_add_history(utf8line + console.input.prompt_len);
	script_repl(utf8line + console.input.prompt_len);
	free(utf8line);
	console.lines.scroll_pos = 0;
	console.input.cursor = console.input.prompt_len;
	input[console.input.cursor] = 0;
	console.history.selected = -1;
	free(console.history.input_saved);
	console.history.input_saved = NULL;
}

static void con_move_cursor(int ofs) {
	console.input.cursor = iclamp((int)console.input.cursor + ofs, console.input.prompt_len, ucs4len(console.input.buffer));
	console.lines.scroll_pos = 0;
}

static void con_scroll(int ofs) {
	console.lines.scroll_pos = iclamp((int)console.lines.scroll_pos + ofs, 0, console.lines.num - 1);
}

static void con_clear(void) {
	console.lines.buffer = realloc(console.lines.buffer, CON_LINELEN);
	console.lines.num = 1;
	console.lines.current.index = 0;
	console.lines.current.cursor = 0;
	console.lines.scroll_pos = 0;
	memset(console.lines.buffer, 0, CON_LINELEN);
	con_print_internal(CON_LINE_PREFIX, "");
}

void con_set_prompt(const char *prompt) {
	uint32_t *input = INPUT_POINTER(0);
	uint32_t pbuf[strlen(prompt) + 1];
	utf8_to_ucs4(prompt, sizeof(pbuf), pbuf);
	int plen = ucs4len(pbuf);

	if(plen != console.input.prompt_len) {
		memmove(input + plen, input + console.input.prompt_len, (ucs4len(input + console.input.prompt_len) + 1) * sizeof(*input));
		console.input.cursor += (plen - (int)console.input.prompt_len);
	}

	memcpy(input, pbuf, sizeof(pbuf) - 1);
	console.input.prompt_len = plen;
}

void con_init(void) {
	console.input.buffer = calloc(sizeof(*console.input.buffer), CON_LINELEN);
	console.history.selected = -1;
	con_clear();
	con_set_prompt("> ");
}

void con_postinit(void) {
	console.res.font = res_ref(RES_FONT, CON_FONT, RESF_LAZY);
	console.res.shader = res_ref(RES_SHADERPROG, "text_default", RESF_LAZY);
}

void con_shutdown(void) {
	res_unref((ResourceRef*)&console.res, sizeof(console.res)/sizeof(ResourceRef));
	free(console.lines.buffer);
	free(console.input.buffer);
	free(console.history.buffer);
	free(console.history.input_saved);
	memset(&console, 0, sizeof(console));
}

void con_update(void) {
	++console.frames;

	events_poll((EventHandler[]) {
		{ con_event, NULL, EPRIO_CAPTURE },
		{ NULL },
	}, EFLAG_TEXT);
}

static void rewrap(const char *text_utf8, uint32_t *buf_ucs4, size_t buf_size, Font *font, double max_width) {
	uint32_t *bufptr = buf_ucs4;
	uint32_t *bufend = buf_ucs4 + buf_size - 1;
	double iscale = 1 / font_get_metrics(font)->scale;
	double width = 0;
	double space_width = font_get_char_metrics(font, ' ')->advance * iscale;
	int line_charnum = 0;

	while(*text_utf8) {
		// NOTE: This can be made much faster if we can assume the font is monospace.

		uint32_t chr = utf8_getch(&text_utf8);

		if(chr == '\t') {
			int numspaces = 8 - (line_charnum % 8);

			do {
				width += space_width;

				if(width > max_width) {
					width = 0;
					*bufptr++ = '\n';
					assert(bufptr < buf_ucs4 + buf_size);

					if(bufptr == bufend) {
						goto end;
					}

					line_charnum = 0;
				}

				*bufptr++ = ' ';
				assert(bufptr < buf_ucs4 + buf_size);

				if(bufptr == bufend) {
					goto end;
				}

				++line_charnum;
			} while(--numspaces);

			continue;
		}

		const GlyphMetrics *m = font_get_char_metrics(font, chr);

		if(m) {
			width += m->advance * iscale;
		}

		if(width > max_width) {
			width = 0;
			*bufptr++ = '\n';
			assert(bufptr < buf_ucs4 + buf_size);

			if(bufptr == bufend) {
				break;
			}

			line_charnum = 0;
		}

		*bufptr++ = chr;
		assert(bufptr < buf_ucs4 + buf_size);

		if(bufptr == bufend) {
			break;
		}

		++line_charnum;
	}

end:
	*bufptr = 0;
}

static void rewrap_ucs4(const uint32_t *text_ucs4, uint32_t *buf_ucs4, size_t buf_size, Font *font, double max_width, uint cursor_pos, double *cur_x, double *cur_y) {
	uint32_t *bufptr = buf_ucs4;
	uint32_t *bufend = buf_ucs4 + buf_size - 1;
	double iscale = 1 / font_get_metrics(font)->scale;
	double width = 0;
	double height = 0;
	double lskip = font_get_lineskip(font);

	for(uint32_t chr, idx = 0; (chr = *text_ucs4); ++text_ucs4, ++idx) {
		// NOTE: This can be made much faster if we can assume the font is monospace.

		// XXX: No support for printing tabs here, but could be added if needed.
		// There's no way to input them currently anyway.

		const GlyphMetrics *m = font_get_char_metrics(font, chr);

		if(m) {
			width += m->advance * iscale;
		}

		if(width > max_width) {
			width = m->advance * iscale;
			height += lskip;

			if(idx == cursor_pos - 1) {
				*cur_x = width;
				*cur_y = height;
			}

			*bufptr++ = '\n';
			assert(bufptr < buf_ucs4 + buf_size);

			if(bufptr == bufend) {
				break;
			}
		} else if(idx == cursor_pos - 1) {
			*cur_x = width;
			*cur_y = height;
		}

		*bufptr++ = chr;
		assert(bufptr < buf_ucs4 + buf_size);

		if(bufptr == bufend) {
			break;
		}

		continue;
	}

	*bufptr = 0;
}

void con_draw(void) {
	Font *font = res_ref_data(console.res.font);
	FloatRect vp, prev_vp;

	const float con_width = SCREEN_W;
	const float con_height = SCREEN_H / 2;
	float con_text_yofs = font_get_descent(font) - 5;
	float con_line_spacing = font_get_lineskip(font);

	r_state_push();

	r_framebuffer(NULL);
	r_framebuffer_viewport_current(NULL, &prev_vp);
	r_capabilities(r_capability_bit(RCAP_CULL_FACE));
	r_cull(CULL_BACK);
	r_blend(BLEND_PREMUL_ALPHA);

	r_mat_mode(MM_MODELVIEW);
	r_mat_push();
	r_mat_identity();
	r_mat_mode(MM_TEXTURE);
	r_mat_push();
	r_mat_identity();
	r_mat_mode(MM_PROJECTION);
	r_mat_push();
	r_mat_ortho(0, SCREEN_W, SCREEN_H, 0, -1, 1);
	video_get_viewport(&vp);
	r_framebuffer_viewport_rect(NULL, vp);

	r_shader_standard_notex();
	r_color4(0.0, 0.0, 0.1, 0.75);
	r_mat_mode(MM_MODELVIEW);

	r_mat_push();
	r_mat_scale(con_width, con_height, 0);
	r_mat_translate(0.5, 0.5, 0);
	r_draw_quad();
	r_mat_pop();

	uint32_t ucs4_buf[CON_LINELEN * 2];

	int line_idx = console.lines.current.index;
	int first_idx = line_idx;
	line_idx -= (int)console.lines.scroll_pos;

	Color clr;
	TextParams tp = {
		.pos = { 0, con_height + con_text_yofs + con_line_spacing },
		.font_ptr = font,
		.shader_ptr = res_ref_data(console.res.shader),
		.align = ALIGN_LEFT,
		.blend = BLEND_PREMUL_ALPHA,
		.color = &clr,
	};

	double cur_x, cur_y;

	if(console.lines.scroll_pos) {
		clr = *RGBA(0.4, 1, 1, 1);
		char buf[64];
		snprintf(buf, sizeof(buf), " --- %i MORE ---", console.lines.scroll_pos);
		rewrap(buf, ucs4_buf, sizeof(ucs4_buf), tp.font_ptr, con_width);
	} else {
		clr = *RGBA(1, 0.4, 0.2, 1);
		rewrap_ucs4(INPUT_POINTER(0), ucs4_buf, sizeof(ucs4_buf), tp.font_ptr, con_width, console.input.cursor, &cur_x, &cur_y);
	}

	tp.pos.y -= text_ucs4_height(tp.font_ptr, ucs4_buf, 0);
	text_ucs4_draw(ucs4_buf, &tp);

	if((console.frames / 30) % 2 && !console.lines.scroll_pos) {
		TextParams tp_cur = tp;
		tp_cur.pos.x = cur_x;
		tp_cur.pos.y += cur_y + 2;
		text_draw("_", &tp_cur);
	}

	clr = *RGBA(0.75, 0.75, 0.75, 1);

	for(;;) {
		if(--line_idx < 0) {
			line_idx = console.lines.num + line_idx;
		}

		if(line_idx == first_idx || line_idx < 0 || tp.pos.y < con_line_spacing) {
			break;
		}

		char *line = LINE_POINTER(line_idx, 0);

		if(*line) {
			rewrap(line, ucs4_buf, sizeof(ucs4_buf), tp.font_ptr, con_width);
			tp.pos.y -= text_ucs4_height(tp.font_ptr, ucs4_buf, 0);
			text_ucs4_draw(ucs4_buf, &tp);
		} else {
			tp.pos.y -= con_line_spacing;
		}
	}

	r_mat_pop();
	r_mat_mode(MM_TEXTURE);
	r_mat_pop();
	r_mat_mode(MM_PROJECTION);
	r_mat_pop();

	r_framebuffer_viewport_rect(NULL, prev_vp);
	r_state_pop();
}

void con_set_active(bool active) {
	console.active = active;
}

bool con_is_active(void) {
	return console.active;
}

static bool con_event(SDL_Event *evt, void *arg) {
	if(!con_is_active()) {
		return false;
	}

	switch(evt->type) {
		case SDL_TEXTINPUT:
			con_insert(evt->text.text);
			break;

		case SDL_KEYDOWN:
			switch(evt->key.keysym.scancode) {
				case SDL_SCANCODE_RETURN:
					con_submit();
					break;

				case SDL_SCANCODE_ESCAPE:
					con_set_active(false);
					break;

				case SDL_SCANCODE_GRAVE:
					if(evt->key.keysym.mod & KMOD_LSHIFT) {
						con_set_active(false);
					}
					break;

				case SDL_SCANCODE_LEFT:
					con_move_cursor(-1);
					break;

				case SDL_SCANCODE_RIGHT:
					con_move_cursor(+1);
					break;

				case SDL_SCANCODE_UP:
					if(evt->key.keysym.mod & KMOD_SHIFT) {
						con_scroll(+1);
					} else {
						con_cycle_history(+1);
					}
					break;

				case SDL_SCANCODE_DOWN:
					if(evt->key.keysym.mod & KMOD_SHIFT) {
						con_scroll(-1);
					} else {
						con_cycle_history(-1);
					}
					break;

				case SDL_SCANCODE_HOME:
					con_move_cursor(-CON_LINELEN);
					break;

				case SDL_SCANCODE_END:
					con_move_cursor(+CON_LINELEN);
					break;

				case SDL_SCANCODE_PAGEUP:
					con_scroll(+5);
					break;

				case SDL_SCANCODE_PAGEDOWN:
					con_scroll(-5);
					break;

				case SDL_SCANCODE_BACKSPACE:
					con_erase(-1);
					break;

				case SDL_SCANCODE_DELETE:
					con_erase(+1);
					break;

				case SDL_SCANCODE_U:
					if(evt->key.keysym.mod & KMOD_CTRL) {
						con_move_cursor(-CON_LINELEN);
						con_erase(+CON_LINELEN);
					}
					break;

				case SDL_SCANCODE_H:
					if(evt->key.keysym.mod & KMOD_CTRL) {
						con_erase(-1);
					}
					break;

				case SDL_SCANCODE_L:
					if(evt->key.keysym.mod & KMOD_CTRL) {
						con_clear();
					}
					break;

				case SDL_SCANCODE_W:
					if(evt->key.keysym.mod & KMOD_CTRL) {
						// TODO: Erase word
					}
					break;

				default: break;
			}
			break;
	}

	return true;
}
