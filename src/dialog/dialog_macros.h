/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_dialog_dialog_macros_h
#define IGUARD_dialog_dialog_macros_h

#include "taisei.h"

#include "dialog.h"
#include "stage.h"

#define DIALOG_BEGIN(_interface) \
	Dialog dialog; \
	stage_begin_dialog(&dialog); \
	_interface##DialogEvents events = { 0 }; \
	COEVENT_INIT_ARRAY(events); \
	if(ARGS.out_events) *ARGS.out_events = &events; \
	YIELD \

#define DIALOG_END() \
	dialog_end(&dialog); \
	COEVENT_CANCEL_ARRAY(events)

#define ACTOR(_name, _side) \
	DialogActor _name; \
	dialog_add_actor(&dialog, &_name, #_name, _side)

#define ACTOR_LEFT(_name) ACTOR(_name, DIALOG_SIDE_LEFT)
#define ACTOR_RIGHT(_name) ACTOR(_name, DIALOG_SIDE_RIGHT)

#define MSG(_actor, _text) dialog_message(&dialog, &_actor, _text)
#define MSG_UNSKIPPABLE(_actor, _delay, _text) dialog_message_unskippable(&dialog, &_actor, _text, _delay)
#define FACE(_actor, _face) dialog_actor_set_face(&_actor, #_face)
#define VARIANT(_actor, _variant) dialog_actor_set_face(&_actor, #_variant)
#define SHOW(_actor) dialog_actor_show(&_actor)
#define HIDE(_actor) dialog_actor_hide(&_actor)
#define FOCUS(_actor) dialog_focus_actor(&dialog, &_actor)
#define WAIT_SKIPPABLE(_delay) dialog_skippable_wait(&dialog, _delay)
#define EVENT(_name) coevent_signal(&events._name)
#define TITLE(_actor, _name, _title) log_warn("TITLE(%s, %s, %s) not yet implemented", #_actor, #_name, #_title)

#define DIALOG_TASK(_protag, _interface) \
	TASK_WITH_INTERFACE(_protag##_##_interface##Dialog, _interface##Dialog)

#endif // IGUARD_dialog_dialog_macros_h
