/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "dynarray.h"

#include "cotask.h"

typedef enum CoEventStatus {
	CO_EVENT_PENDING,
	CO_EVENT_SIGNALED,
	CO_EVENT_CANCELED,
} CoEventStatus;

typedef struct CoEvent {
	DYNAMIC_ARRAY(BoxedTask) subscribers;
	uint32_t unique_id;
	uint32_t num_signaled;
} CoEvent;

typedef struct CoEventSnapshot {
	uint32_t unique_id;
	uint32_t num_signaled;
} CoEventSnapshot;

#define COEVENTS_ARRAY(...) \
	union { \
		CoEvent _first_event_; \
		struct { CoEvent __VA_ARGS__; }; \
	}

typedef COEVENTS_ARRAY(
	finished
) CoTaskEvents;

void coevent_init(CoEvent *evt);
void coevent_signal(CoEvent *evt);
void coevent_signal_once(CoEvent *evt);
void coevent_cancel(CoEvent *evt);
CoEventSnapshot coevent_snapshot(const CoEvent *evt);
CoEventStatus coevent_poll(const CoEvent *evt, const CoEventSnapshot *snap);

void _coevent_array_action(uint num, CoEvent *events, void (*func)(CoEvent*));

#define COEVENT_ARRAY_ACTION(func, array) \
	(_coevent_array_action(sizeof(array)/sizeof(CoEvent), &((array)._first_event_), func))
#define COEVENT_INIT_ARRAY(array) COEVENT_ARRAY_ACTION(coevent_init, array)
#define COEVENT_CANCEL_ARRAY(array) COEVENT_ARRAY_ACTION(coevent_cancel, array)
