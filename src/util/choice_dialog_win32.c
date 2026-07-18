/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2026, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2026, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "choice_dialog.h"
#include "util/stringops.h"

#include <windows.h>
#include <string.h>
#include <wchar.h>

/*
 * DISCLAIMER: This is mostly vibecoded, because i didn't want to bother learning the win32 API for this crap.
 * If you have a hard stance against AI slop, then it's *really* time to stop using windows.
 * I don't love it either, but even i still somewhat value my sanity. Which is why i choose to put as much distance
 * between myself and insane windows-exclusive workarounds as possible. Notice how there's no implementation of this
 * for any other platform? That's because other platforms don't force us to ask the user stupid fucking questions before
 * the game can even start rendering itself.
 */

/* ---- string helpers ---------------------------------------------------*/

static WCHAR *utf8_to_wchar(MemArena *arena, const char *utf8) {
	if(!utf8) {
		return NULL;
	}

	auto sz = strlen(utf8) + 1;
	auto w = ARENA_ALLOC_ARRAY(arena, sz, WCHAR);
	utf8_to_utf16(utf8, sz, w);

	return w;
}

/* ---- window plumbing ---------------------------------------------------*/

enum {
	IDC_RADIO_BASE = 100, /* IDC_RADIO_BASE .. IDC_RADIO_BASE + num_choices - 1 */
};

typedef struct DialogBuildInfo {
	const WCHAR *text;
	const WCHAR *ok_label;
	const WCHAR *cancel_label;
	WCHAR **choices;
	unsigned int num_choices;
	unsigned int initial_selection;
} DialogBuildInfo;

typedef struct DialogContext {
	HWND *radio_buttons;
	unsigned int num_choices;
	HWND ok_button;
	HWND cancel_button;
	HFONT font;
	int result;
} DialogContext;

typedef struct CreateParams {
	const DialogBuildInfo *build;
	DialogContext *ctx;
} CreateParams;

static int scale_for_dpi(int value, UINT dpi) {
	return MulDiv(value, (int)dpi, USER_DEFAULT_SCREEN_DPI);
}

static void build_dialog_controls(HWND hwnd, HINSTANCE hinstance, const DialogBuildInfo *build, DialogContext *ctx) {
	enum {
		MARGIN              = 12,
		CONTENT_WIDTH       = 420,
		GAP_AFTER_TEXT      = 16,
		GAP_BETWEEN_RADIO   = 6,
		GAP_BEFORE_BUTTONS  = 18,
		BUTTON_HEIGHT_MIN   = 28,
		BUTTON_GAP          = 10,
		BUTTON_PAD_X        = 24,
		BUTTON_PAD_Y        = 6,
		BUTTON_MIN_WIDTH    = 84,
		RADIO_INDENT        = 24,
		LINE_MIN_HEIGHT     = 20,
	};

	/*
	 * The constants above are design values at 96 DPI (100% scaling).
	 * SystemParametersInfoForDpi() below hands back a font already
	 * scaled for this window's actual monitor, so the layout has to be
	 * scaled to match, or a bigger font just overflows a box sized for
	 * a smaller one.
	 */
	UINT dpi = GetDpiForWindow(hwnd);

	int margin             = scale_for_dpi(MARGIN, dpi);
	int content_width      = scale_for_dpi(CONTENT_WIDTH, dpi);
	int gap_after_text     = scale_for_dpi(GAP_AFTER_TEXT, dpi);
	int gap_between_radio  = scale_for_dpi(GAP_BETWEEN_RADIO, dpi);
	int gap_before_buttons = scale_for_dpi(GAP_BEFORE_BUTTONS, dpi);
	int button_height_min  = scale_for_dpi(BUTTON_HEIGHT_MIN, dpi);
	int button_gap         = scale_for_dpi(BUTTON_GAP, dpi);
	int button_pad_x       = scale_for_dpi(BUTTON_PAD_X, dpi);
	int button_pad_y       = scale_for_dpi(BUTTON_PAD_Y, dpi);
	int button_min_width   = scale_for_dpi(BUTTON_MIN_WIDTH, dpi);
	int radio_indent       = scale_for_dpi(RADIO_INDENT, dpi);
	int line_min_height    = scale_for_dpi(LINE_MIN_HEIGHT, dpi);

	NONCLIENTMETRICSW ncm;
	ncm.cbSize = sizeof(ncm);
	if(SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0, dpi)) {
		ctx->font = CreateFontIndirectW(&ncm.lfMessageFont);
	}

	HDC hdc = GetDC(hwnd);
	HFONT old_font = NULL;
	if(ctx->font) {
		old_font = SelectObject(hdc, ctx->font);
	}

	int y = margin;

	/* message text, word-wrapped to fit the content width */
	RECT text_rc = { 0, 0, content_width - 2 * margin, 0 };
	DrawTextW(hdc, build->text, -1, &text_rc, DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);
	int text_h = text_rc.bottom - text_rc.top;
	if(text_h < line_min_height) {
		text_h = line_min_height;
	}

	HWND h_text = CreateWindowExW(0, L"STATIC", build->text,
		WS_CHILD | WS_VISIBLE | SS_LEFT,
		margin, y, content_width - 2 * margin, text_h,
		hwnd, NULL, hinstance, NULL);
	if(ctx->font) {
		SendMessageW(h_text, WM_SETFONT, (WPARAM)ctx->font, TRUE);
	}
	y += text_h + gap_after_text;

	/* one BS_AUTORADIOBUTTON per choice, each individually word-wrapped
	 * and sized so long option text doesn't get clipped */
	int radio_width = content_width - 2 * margin - radio_indent;
	for(unsigned int i = 0; i < build->num_choices; ++i) {
		RECT item_rc = { 0, 0, radio_width, 0 };
		DrawTextW(hdc, build->choices[i], -1, &item_rc, DT_CALCRECT | DT_WORDBREAK | DT_NOPREFIX);
		int item_h = item_rc.bottom - item_rc.top;
		if(item_h < line_min_height) {
			item_h = line_min_height;
		}

		DWORD style = WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTORADIOBUTTON | BS_MULTILINE;
		if(i == 0) {
			style |= WS_GROUP;
		}

		HWND h_radio = CreateWindowExW(0, L"BUTTON", build->choices[i],
			style,
			margin + radio_indent, y, radio_width, item_h,
			hwnd, (HMENU)(UINT_PTR)(IDC_RADIO_BASE + i), hinstance, NULL);
		if(ctx->font) {
			SendMessageW(h_radio, WM_SETFONT, (WPARAM)ctx->font, TRUE);
		}

		ctx->radio_buttons[i] = h_radio;
		y += item_h + gap_between_radio;
	}

	if(build->num_choices > 0) {
		unsigned int sel = build->initial_selection < build->num_choices ? build->initial_selection : 0;
		SendMessageW(ctx->radio_buttons[sel], BM_SETCHECK, BST_CHECKED, 0);
	}

	y += gap_before_buttons - gap_between_radio;

	/* OK / Cancel, auto-widened and auto-heightened to fit their labels */
	SIZE ok_extent = { 0, 0 };
	GetTextExtentPoint32W(hdc, build->ok_label, (int)wcslen(build->ok_label), &ok_extent);
	SIZE cancel_extent = { 0, 0 };
	GetTextExtentPoint32W(hdc, build->cancel_label, (int)wcslen(build->cancel_label), &cancel_extent);

	int ok_width = ok_extent.cx + button_pad_x;
	if(ok_width < button_min_width) {
		ok_width = button_min_width;
	}
	int cancel_width = cancel_extent.cx + button_pad_x;
	if(cancel_width < button_min_width) {
		cancel_width = button_min_width;
	}

	int button_height = ok_extent.cy > cancel_extent.cy ? ok_extent.cy : cancel_extent.cy;
	button_height += button_pad_y * 2;
	if(button_height < button_height_min) {
		button_height = button_height_min;
	}

	int cancel_x = content_width - margin - cancel_width;
	int ok_x     = cancel_x - button_gap - ok_width;

	ctx->ok_button = CreateWindowExW(0, L"BUTTON", build->ok_label,
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_GROUP | BS_DEFPUSHBUTTON,
		ok_x, y, ok_width, button_height,
		hwnd, (HMENU)(UINT_PTR)IDOK, hinstance, NULL);
	if(ctx->font) {
		SendMessageW(ctx->ok_button, WM_SETFONT, (WPARAM)ctx->font, TRUE);
	}

	ctx->cancel_button = CreateWindowExW(0, L"BUTTON", build->cancel_label,
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
		cancel_x, y, cancel_width, button_height,
		hwnd, (HMENU)(UINT_PTR)IDCANCEL, hinstance, NULL);
	if(ctx->font) {
		SendMessageW(ctx->cancel_button, WM_SETFONT, (WPARAM)ctx->font, TRUE);
	}

	y += button_height + margin;

	if(old_font) {
		SelectObject(hdc, old_font);
	}
	ReleaseDC(hwnd, hdc);

	/* the window was created with a placeholder size/position; now that
	 * the content is measured, resize and re-center it for real */
	RECT client_rc = { 0, 0, content_width, y };
	DWORD wnd_style   = (DWORD)GetWindowLongPtrW(hwnd, GWL_STYLE);
	DWORD wnd_exstyle = (DWORD)GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
	AdjustWindowRectEx(&client_rc, wnd_style, FALSE, wnd_exstyle);

	int win_w = client_rc.right - client_rc.left;
	int win_h = client_rc.bottom - client_rc.top;
	int win_x = (GetSystemMetrics(SM_CXSCREEN) - win_w) / 2;
	int win_y = (GetSystemMetrics(SM_CYSCREEN) - win_h) / 2;

	SetWindowPos(hwnd, NULL, win_x, win_y, win_w, win_h, SWP_NOZORDER | SWP_NOACTIVATE);
}

static LRESULT CALLBACK radio_dialog_wndproc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	switch(msg) {
		case WM_CREATE: {
			CREATESTRUCTW *cs = (CREATESTRUCTW *)lparam;
			CreateParams *cp  = (CreateParams *)cs->lpCreateParams;
			build_dialog_controls(hwnd, cs->hInstance, cp->build, cp->ctx);
			SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)cp->ctx);
			return 0;
		}

		case DM_GETDEFID:
			/* plain WNDCLASS windows don't answer this on their own the
			 * way DefDlgProc() would; IsDialogMessage() needs it to know
			 * that Enter should activate the OK button */
			return MAKELONG(IDOK, DC_HASDEFID);

		case WM_COMMAND: {
			DialogContext *ctx = (DialogContext *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
			if(!ctx) {
				break;
			}

			WORD id   = LOWORD(wparam);
			WORD code = HIWORD(wparam);
			if(code != BN_CLICKED) {
				break;
			}

			if(id == IDOK) {
				for(unsigned int i = 0; i < ctx->num_choices; ++i) {
					if(SendMessageW(ctx->radio_buttons[i], BM_GETCHECK, 0, 0) == BST_CHECKED) {
						ctx->result = (int)i;
						break;
					}
				}
				DestroyWindow(hwnd);
				return 0;
			}

			if(id == IDCANCEL) {
				DestroyWindow(hwnd);
				return 0;
			}

			break;
		}

		case WM_CLOSE:
			DestroyWindow(hwnd);
			return 0;
	}

	return DefWindowProcW(hwnd, msg, wparam, lparam);
}

static int run_radio_dialog(const WCHAR *title, CreateParams *cp, DialogContext *ctx) {
	static const WCHAR CLASS_NAME[] = L"TaiseiRadioDialogWindowClass";
	HINSTANCE hinstance = GetModuleHandleW(NULL);

	/*
	 * Only load icons and register the class the first time this is
	 * called. RegisterClassExW() silently no-ops if the class already
	 * exists, so without this check every repeat call would load a
	 * fresh pair of icon handles and never use or free them.
	 */
	WNDCLASSEXW existing_wc;
	if(!GetClassInfoExW(hinstance, CLASS_NAME, &existing_wc)) {
		HICON icon_large = (HICON)LoadImageW(hinstance, L"IDI_ICON1", IMAGE_ICON,
			GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0);
		if(!icon_large) {
			icon_large = LoadIconW(NULL, IDI_APPLICATION);
		}

		HICON icon_small = (HICON)LoadImageW(hinstance, L"IDI_ICON1", IMAGE_ICON,
			GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
		if(!icon_small) {
			icon_small = icon_large;
		}

		WNDCLASSEXW wc;
		ZeroMemory(&wc, sizeof(wc));
		wc.cbSize        = sizeof(wc);
		wc.style         = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc   = radio_dialog_wndproc;
		wc.hInstance     = hinstance;
		wc.hCursor       = LoadCursorW(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
		wc.lpszClassName = CLASS_NAME;
		wc.hIcon         = icon_large;
		wc.hIconSm       = icon_small;

		if(!RegisterClassExW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
			return -1;
		}
	}

	DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN;

	HWND hwnd = CreateWindowExW(0, CLASS_NAME, title,
		style, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, hinstance, cp);

	if(!hwnd) {
		return -1;
	}

	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);
	SetForegroundWindow(hwnd);
	SetFocus(ctx->ok_button);

	MSG msg;
	while(IsWindow(hwnd)) {
		int r = GetMessageW(&msg, NULL, 0, 0);
		if(r <= 0) {
			/* WM_QUIT (0) or a GetMessage error (-1) reached this thread.
			 * Don't swallow a genuine shutdown request: repost it so the
			 * caller's own message loop still sees it once we return. */
			if(r == 0) {
				PostQuitMessage((int)msg.wParam);
			}
			break;
		}

		if(!IsDialogMessageW(hwnd, &msg)) {
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
	}

	if(IsWindow(hwnd)) {
		DestroyWindow(hwnd);
	}

	if(ctx->font) {
		DeleteObject(ctx->font);
	}

	return ctx->result;
}

/* ---- public entry point ------------------------------------------------*/

int show_choice_dialog(MemArena *arena, const ChoiceDialogParams *params) {
	if(!arena || !params || params->num_choices == 0 || !params->choices) {
		return -1;
	}

	WCHAR *title_w  = utf8_to_wchar(arena, params->window_title ? params->window_title : "");
	WCHAR *ok_w     = utf8_to_wchar(arena, params->ok_label ? params->ok_label : "OK");
	WCHAR *cancel_w = utf8_to_wchar(arena, params->cancel_label ? params->cancel_label : "Cancel");
	WCHAR *text_w   = utf8_to_wchar(arena, params->text ? params->text : "");

	WCHAR **choices_w = marena_alloc_array(arena, params->num_choices, sizeof(WCHAR *));
	if(choices_w) {
		for(unsigned int i = 0; i < params->num_choices; ++i) {
			choices_w[i] = utf8_to_wchar(arena, params->choices[i] ? params->choices[i] : "");
		}
	}

	BOOL ready = title_w && ok_w && cancel_w && text_w && choices_w;
	if(ready) {
		for(unsigned int i = 0; i < params->num_choices; ++i) {
			if(!choices_w[i]) {
				ready = FALSE;
				break;
			}
		}
	}

	if(!ready) {
		return -1;
	}

	DialogContext ctx;
	ZeroMemory(&ctx, sizeof(ctx));
	ctx.result        = -1;
	ctx.num_choices   = params->num_choices;
	ctx.radio_buttons = marena_alloc_array(arena, params->num_choices, sizeof(HWND));

	if(!ctx.radio_buttons) {
		return -1;
	}

	DialogBuildInfo build;
	build.text              = text_w;
	build.ok_label          = ok_w;
	build.cancel_label      = cancel_w;
	build.choices           = choices_w;
	build.num_choices       = params->num_choices;
	build.initial_selection = params->initial_selection;

	CreateParams cp;
	cp.build = &build;
	cp.ctx   = &ctx;

	return run_radio_dialog(title_w, &cp, &ctx);
}
