/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 * Copyright (C) 2011, Alexeyew Andrew <https://github.com/nexAkari>
 */

#ifndef OPTMENU_H
#define OPTMENU_H

#include "menu.h"

void create_options_menu(MenuData *m);

void draw_options_menu(MenuData *m);
int options_menu_loop(MenuData *m);

typedef int (*BindingGetter)(void*);
typedef int (*BindingSetter)(void*, int);
typedef int (*BindingDependence)();

typedef enum BindingType {
	BT_IntValue,
	BT_KeyBinding,
	BT_StrValue,
	BT_Resolution
} BindingType;

typedef struct OptionBinding {
	char **values;
	int valcount;
	int valrange_min;
	int valrange_max;
	BindingGetter getter;
	BindingSetter setter;
	BindingDependence dependence;
	int selected;
	int configentry;
	int enabled;
	char *optname;
	BindingType type;
	int blockinput;
} OptionBinding;

void draw_options_menu_bg(MenuData*);

#endif

