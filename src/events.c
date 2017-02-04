/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2012, Alexeyew Andrew <http://akari.thebadasschoobs.org/>
 */

#include "events.h"
#include "config.h"
#include "global.h"
#include "video.h"
#include "gamepad.h"

void handle_events(EventHandler handler, EventFlags flags, void *arg) {
	SDL_Event event;
	const Uint8 *keys = SDL_GetKeyboardState(NULL);
	
	int kbd 	= flags & EF_Keyboard;
	int text	= flags & EF_Text;
	int menu	= flags & EF_Menu;
	int game	= flags & EF_Game;

	// TODO: rewrite text input handling to properly support multibyte characters and IMEs

	if(text) {
		if(!SDL_IsTextInputActive()) {
			SDL_StartTextInput();
		}
	} else {
		if(SDL_IsTextInputActive()) {
			SDL_StopTextInput();
		}
	}

	while(SDL_PollEvent(&event)) {
		int sym = event.key.keysym.sym;
		
		switch(event.type) {
			case SDL_KEYDOWN:
				if(sym == tconfig.intval[KEY_SCREENSHOT]) {
					take_screenshot();
					break;
				}

				if((sym == SDLK_RETURN && (keys[SDL_SCANCODE_LALT] || keys[SDL_SCANCODE_RALT])) || sym == tconfig.intval[KEY_FULLSCREEN]) {
					video_toggle_fullscreen();
					break;
				}

				if(kbd)
					handler(E_KeyDown, sym, arg);
				
				if(menu) {
					if(sym == tconfig.intval[KEY_DOWN] || sym == SDLK_DOWN) {
						handler(E_CursorDown, 0, arg);
					} else if(sym == tconfig.intval[KEY_UP] || sym == SDLK_UP) {
						handler(E_CursorUp, 0, arg);
					} else if(sym == tconfig.intval[KEY_RIGHT] || sym == SDLK_RIGHT) {
						handler(E_CursorRight, 0, arg);
					} else if(sym == tconfig.intval[KEY_LEFT] || sym == SDLK_LEFT) {
						handler(E_CursorLeft, 0, arg);
					} else if(sym == tconfig.intval[KEY_SHOT] || sym == SDLK_RETURN) {
						handler(E_MenuAccept, 0, arg);
					} else if(sym == SDLK_ESCAPE || sym == tconfig.intval[KEY_BOMB]) {
						handler(E_MenuAbort, 0, arg);
					}
				}
				
				if(game) {
					if(sym == SDLK_ESCAPE)
						handler(E_Pause, 0, arg);
					else {
						int key = config_sym2key(sym);
						if(key >= 0)
							handler(E_PlrKeyDown, key, arg);
					}
				}
				
				if(text) {
					if(sym == SDLK_ESCAPE)
						handler(E_CancelText, 0, arg);
					else if(sym == SDLK_RETURN)
						handler(E_SubmitText, 0, arg);
					else if(sym == SDLK_BACKSPACE)
						handler(E_CharErased, 0, arg);
				}
				
				break;
			
			case SDL_KEYUP:
				if(kbd)
					handler(E_KeyUp, sym, arg);
				
				if(game) {
					int key = config_sym2key(sym);
					if(key >= 0)
						handler(E_PlrKeyUp, key, arg);
				}
				
				break;
			
			case SDL_TEXTINPUT: {
				char *c;

				for(c = event.text.text; *c; ++c) {
					handler(E_CharTyped, *c, arg);
				}

				break;
			}

			case SDL_QUIT:
				exit(0);
				break;
			
			default:
				gamepad_event(&event, handler, flags, arg);
				break;
		}
	}
}
