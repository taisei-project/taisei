/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

%{	
	#include "config.h"
	#include <stdio.h>
	#include <string.h>
	#include <stdlib.h>
	
	#include "paths/native.h"
	#include "taisei_err.h"
		
	Config tconfig;
	int lineno;
	
	int yywrap() {
		return 1;
	}
	
	int yyerror(char *s) {
		errx(-1, "!- %d: %s", lineno, s);
		return 1;
	}
	
	extern int yylex(void);
	extern FILE *yyin;
%}

%token tKEY_UP
%token tKEY_DOWN
%token tKEY_LEFT
%token tKEY_RIGHT

%token tKEY_FOCUS

%token tKEY_SHOT
%token tKEY_BOMB

%token tKEY_FULLSCREEN
%token tKEY_SCREENSHOT

%token tFULLSCREEN

%token tNO_SHADER
%token tNO_AUDIO
%token tNO_STAGEBG
%token tNO_STAGEBG_FPSLIMIT

%token SKEY

%token NUMBER
%token tCHAR

%token SEMI
%token EQ
%token LB

%%

file	: line nl file
		| nl file
		| ;
	
line	: key_key EQ key_val {
			if($1 > sizeof(tconfig.intval)/sizeof(int))
				errx(-1, "config index out of range"); // should not happen
			tconfig.intval[$1] = $3;
		};

key_val : SKEY
		| NUMBER
		| tCHAR;

key_key	: tKEY_UP
		| tKEY_DOWN
		| tKEY_LEFT
		| tKEY_RIGHT
		| tKEY_FOCUS
		| tKEY_SHOT
		| tKEY_BOMB
		| tKEY_FULLSCREEN
		| tKEY_SCREENSHOT
		| tNO_SHADER
		| tNO_AUDIO
		| tFULLSCREEN
		| tNO_STAGEBG
		| tNO_STAGEBG_FPSLIMIT;

nl		: LB { lineno++; };
%%

void parse_config(char *filename) {
	config_preset();
		
	lineno = 1;
	
	char *buf;
	
	buf = malloc(strlen(filename)+strlen(get_config_path())+3);	
	strcpy(buf, get_config_path());
	
	strcat(buf, "/");
	strcat(buf, filename);
	
	yyin = fopen(buf, "r");
	
	printf("parse_config():\n");
	if(yyin) {
		yyparse();
		printf("-- parsing complete\n");
		fclose(yyin);
	} else {
		printf("-- parsing incomplete; falling back to built-in preset\n");
		warnx("problems with parsing %s", buf);
	}
	free(buf);
}

void config_preset() {
	tconfig.intval[KEY_UP] = SDLK_UP;
	tconfig.intval[KEY_DOWN] = SDLK_DOWN;
	tconfig.intval[KEY_LEFT] = SDLK_LEFT;
	tconfig.intval[KEY_RIGHT] = SDLK_RIGHT;
	
	tconfig.intval[KEY_FOCUS] = SDLK_LSHIFT;
	tconfig.intval[KEY_SHOT] = SDLK_z;
	tconfig.intval[KEY_BOMB] = SDLK_x;
	
	tconfig.intval[KEY_FULLSCREEN] = SDLK_F11;
	tconfig.intval[KEY_SCREENSHOT] = SDLK_p;
	
	tconfig.intval[FULLSCREEN] = 0;
	
	tconfig.intval[NO_SHADER] = 0;
	tconfig.intval[NO_AUDIO] = 0;
	
	tconfig.intval[NO_STAGEBG] = 0;
	tconfig.intval[NO_STAGEBG_FPSLIMIT] = 40;
}

int config_sym2key(int sym) {
	int i;
	for(i = CONFIG_KEY_FIRST; i <= CONFIG_KEY_LAST; ++i)
		if(sym == tconfig.intval[i])
			return i;
}
