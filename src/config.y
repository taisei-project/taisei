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
	
	Config tconfig;
	int lineno;
	
	int yywrap() {
		return 1;
	}
	
	int yyerror(char *s) {
		errx(-1, "!- %d: %s", lineno, s);
	}
	
	extern FILE *yyin;
%}

%token tKEY_UP
%token tKEY_DOWN
%token tKEY_LEFT
%token tKEY_RIGHT

%token tKEY_FOCUS

%token tKEY_SHOT
%token tKEY_BOMB


%token SKEY

%token NUMBER
%token tCHAR

%token SEMI
%token EQ
%token LB

%%

file	: line file
		| ;
	
line	: line nl
		| key_key EQ key_val {
			tconfig.intval[$1] = $3;
		}
		| nl;

key_val : SKEY 
		| tCHAR;

key_key	: tKEY_UP
		| tKEY_DOWN
		| tKEY_LEFT
		| tKEY_RIGHT
		| tKEY_FOCUS
		| tKEY_SHOT
		| tKEY_BOMB;

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
}