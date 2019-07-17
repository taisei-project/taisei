/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#ifndef IGUARD_coroutine_h
#define IGUARD_coroutine_h

#include "taisei.h"

#include "entity.h"
#include <koishi.h>

typedef struct CoTask CoTask;
typedef struct CoSched CoSched;
typedef void *(*CoTaskFunc)(void *arg);

// target for the INVOKE_TASK macro
extern CoSched *_cosched_global;

typedef enum CoStatus {
	CO_STATUS_SUSPENDED = KOISHI_SUSPENDED,
	CO_STATUS_RUNNING   = KOISHI_RUNNING,
	CO_STATUS_DEAD      = KOISHI_DEAD,
} CoStatus;

typedef struct CoEvent {
	ListAnchor subscribers;
	uint32_t unique_id;
	uint32_t num_signaled;
} CoEvent;

void coroutines_init(void);
void coroutines_shutdown(void);

CoTask *cotask_new(CoTaskFunc func);
void cotask_free(CoTask *task);
void *cotask_resume(CoTask *task, void *arg);
void *cotask_yield(void *arg);
bool cotask_wait_event(CoEvent *evt, void *arg);
CoStatus cotask_status(CoTask *task);
CoTask *cotask_active(void);
void cotask_bind_to_entity(CoTask *task, EntityInterface *ent);

void coevent_init(CoEvent *evt);
void coevent_signal(CoEvent *evt);
void coevent_cancel(CoEvent *evt);

CoSched *cosched_new(void);
void *cosched_new_task(CoSched *sched, CoTaskFunc func, void *arg);  // creates and runs the task, returns whatever it yields, schedules it for resume on cosched_run_tasks if it's still alive
uint cosched_run_tasks(CoSched *sched);  // returns number of tasks ran
void cosched_free(CoSched *sched);

static inline attr_must_inline void cosched_set_invoke_target(CoSched *sched) { _cosched_global = sched; }

#define TASK(name, argstruct) \
	static char COTASK_UNUSED_CHECK_##name; \
	typedef struct argstruct COARGS_##name; \
	static void COTASK_##name(COARGS_##name ARGS); \
	attr_unused static void *COTASKTHUNK_##name(void *arg) { \
		COTASK_##name(*(COARGS_##name*)arg); \
		return NULL; \
	} \
	typedef struct { CoEvent *event; COARGS_##name args; } COARGSCOND_##name; \
	attr_unused static void *COTASKTHUNKCOND_##name(void *arg) { \
		COARGSCOND_##name args = *(COARGSCOND_##name*)arg; \
		if(WAIT_EVENT(args.event)) { \
			COTASK_##name(args.args); \
		} \
		return NULL; \
	} \
	static void COTASK_##name(COARGS_##name ARGS)

#define INVOKE_TASK(name, ...) do { \
	(void)COTASK_UNUSED_CHECK_##name; \
	COARGS_##name _coargs = { __VA_ARGS__ }; \
	cosched_new_task(_cosched_global, COTASKTHUNK_##name, &_coargs); \
} while(0)

#define INVOKE_TASK_WHEN(event, name, ...) do { \
	(void)COTASK_UNUSED_CHECK_##name; \
	COARGSCOND_##name _coargs = { event, { __VA_ARGS__ } }; \
	cosched_new_task(_cosched_global, COTASKTHUNKCOND_##name, &_coargs); \
} while(0)

#define YIELD         cotask_yield(NULL)
#define WAIT(delay)   do { int _delay = (delay); while(_delay-- > 0) YIELD; } while(0)
#define WAIT_EVENT(e) cotask_wait_event((e), NULL)

// to use these inside a coroutine, define a BREAK_CONDITION macro and a BREAK label.
#define CHECK_BREAK   do { if(BREAK_CONDITION) goto BREAK; } while(0)
#define BYIELD        do { YIELD; CHECK_BREAK; } while(0)
#define BWAIT(frames) do { WAIT(frames); CHECK_BREAK; } while(0)

#define TASK_BIND(ent) cotask_bind_to_entity(cotask_active(), &ent->entity_interface)

#endif // IGUARD_coroutine_h
