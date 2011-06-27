/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

%{
	#include "parser.h"
	#include "config.h"
	#include <SDL/SDL_keysym.h>
	
%}

%%

=			return EQ;

#.+			;

"key_up"	{ yylval = KEY_UP; return tKEY_UP; }
"key_down"	{ yylval = KEY_DOWN; return tKEY_DOWN; }
"key_left"	{ yylval = KEY_LEFT; return tKEY_LEFT; }
"key_right"	{ yylval = KEY_RIGHT; return tKEY_RIGHT; }

"key_focus"	{ yylval = KEY_FOCUS; return tKEY_FOCUS; }

"key_shot"	{ yylval = KEY_SHOT; return tKEY_SHOT; }
"key_bomb"	{ yylval = KEY_BOMB; return tKEY_BOMB; }

"disable_shader" { yylval = NO_SHADER; return tNO_SHADER; }
"disable_audio" {yylval = NO_AUDIO; return tNO_AUDIO; }

"shift"		{ yylval = SDLK_LSHIFT; return SKEY; }
"ctrl"		{ yylval = SDLK_LCTRL; return SKEY; }
"return"	{ yylval = SDLK_RETURN; return SKEY; }
"alt"		{ yylval = SDLK_LALT; return SKEY; }

"up"		{ yylval = SDLK_UP; return SKEY; }
"down"		{ yylval = SDLK_DOWN; return SKEY; }
"right"		{ yylval = SDLK_RIGHT; return SKEY; }
"left"		{ yylval = SDLK_LEFT; return SKEY; }


[0-9]+		{ yylval = atoi(yytext); return NUMBER; }
[a-zA-Z]	{ yylval = yytext[0]; return tCHAR; }

ä			{ yylval = SDLK_WORLD_68; return tCHAR; }
ü			{ yylval = SDLK_WORLD_92; return tCHAR; }
ö			{ yylval = SDLK_WORLD_86; return tCHAR; }
ß			{ yylval = SDLK_WORLD_63; return tCHAR; }

K[0-9]+		{ yylval = atoi(yytext + 1); return tCHAR; }

\n			return LB;
[ \t]		;

%%
