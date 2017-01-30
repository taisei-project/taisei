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
typedef int (*BindingDependence)(void);

typedef void (*FloatSetter)(float);

typedef enum BindingType {
	BT_IntValue,
	BT_KeyBinding,
	BT_StrValue,
	BT_Resolution,
	BT_Scale,
	BT_ScaleCallback,
	BT_GamepadKeyBinding
} BindingType;

typedef struct OptionBinding {
	char **values;
	int displaysingle;
	int valcount;
	int valrange_min;
	int valrange_max;
	float scale_min;
	float scale_max;
	float scale_step;
	BindingGetter getter;
	BindingSetter setter;
	FloatSetter callback;
	BindingDependence dependence;
	int selected;
	int configentry;
	BindingType type;
	int blockinput;
} OptionBinding;

void draw_options_menu_bg(MenuData*);

#endif

