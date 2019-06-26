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

typedef struct CoTask CoTask;
typedef struct CoSched CoSched;
typedef void (*CoTaskFunc)(void *arg);

// target for the INVOKE_TASK macro
extern CoSched *_cosched_global;

typedef enum CoStatus {
	CO_STATUS_SUSPENDED,
	CO_STATUS_RUNNING,
	CO_STATUS_DEAD,
} CoStatus;

void coroutines_init(void);

CoTask *cotask_new(CoTaskFunc func);
void cotask_free(CoTask *task);
void *cotask_resume(CoTask *task, void *arg);
void *cotask_yield(void *arg);
CoStatus cotask_status(CoTask *task);

CoSched *cosched_new(void);
void *cosched_newtask(CoSched *sched, CoTaskFunc func, void *arg);  // creates and runs the task, returns whatever it yields, schedules it for resume on cosched_run_tasks if it's still alive
uint cosched_run_tasks(CoSched *sched);  // returns number of tasks ran
void cosched_free(CoSched *sched);

static inline attr_must_inline void cosched_set_invoke_target(CoSched *sched) { _cosched_global = sched; }

#define TASK(name, argstruct) \
	typedef struct argstruct COARGS_##name; \
	static void COTASK_##name(COARGS_##name ARGS); \
	static void COTASKTHUNK_##name(void *arg) { \
		COTASK_##name(*(COARGS_##name*)arg); \
	} \
	static void COTASK_##name(COARGS_##name ARGS)

#define INVOKE_TASK(name, ...) do { \
	COARGS_##name _coargs = { __VA_ARGS__ }; \
	cosched_newtask(_cosched_global, COTASKTHUNK_##name, &_coargs); \
} while(0)

#define YIELD cotask_yield(NULL)
#define WAIT(delay) do { int _delay = (delay); do YIELD; while(_delay--); } while(0)

#endif // IGUARD_coroutine_h
