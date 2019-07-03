/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "taisei.h"

#include "coroutine.h"
#include "util.h"

#include <libco.h>

#define CO_STACK_SIZE (4096*2)

struct CoTask {
	LIST_INTERFACE(CoTask);
	cothread_t cothread;
	CoTask *caller;
	CoTaskFunc func;
	void *arg;
	CoStatus status;
};

struct CoSched {
	LIST_ANCHOR(CoTask) tasks, pending_tasks;
};

static CoTask *this_task, main_task;
CoSched *_cosched_global;

static noreturn void co_thunk(void) {
	CoTask *task = this_task;
	task->func(task->arg);
	task->status = CO_STATUS_DEAD;
	cotask_resume(&main_task, NULL);
	UNREACHABLE;
}

CoTask *cotask_new(CoTaskFunc func) {
	CoTask *task = calloc(1, sizeof(*task));
	task->cothread = co_create(CO_STACK_SIZE, co_thunk);
	task->status = CO_STATUS_SUSPENDED;
	task->func = func;
	return task;
}

void cotask_free(CoTask *task) {
	co_delete(task->cothread);
	free(task);
}

void *cotask_resume(CoTask *task, void *arg) {
	assert(task->status == CO_STATUS_SUSPENDED);

	if(this_task->status != CO_STATUS_DEAD) {
		assert(this_task->status == CO_STATUS_RUNNING);
		this_task->status = CO_STATUS_SUSPENDED;
	}

	task->status = CO_STATUS_RUNNING;
	task->caller = this_task;
	task->arg = arg;
	this_task = task;

	co_switch(task->cothread);
	return this_task->arg;
}

void *cotask_yield(void *arg) {
	CoTask *task = this_task->caller;

	assert(task->status == CO_STATUS_SUSPENDED);

	if(this_task->status != CO_STATUS_DEAD) {
		assert(this_task->status == CO_STATUS_RUNNING);
		this_task->status = CO_STATUS_SUSPENDED;
	}

	task->status = CO_STATUS_RUNNING;
	task->arg = arg;
	this_task = task;

	co_switch(task->cothread);
	return this_task->arg;
}

CoStatus cotask_status(CoTask *task) {
	return task->status;
}

CoSched *cosched_new(void) {
	CoSched *sched = calloc(1, sizeof(*sched));
	return sched;
}

void *cosched_new_task(CoSched *sched, CoTaskFunc func, void *arg) {
	CoTask *task = cotask_new(func);
	void *ret = cotask_resume(task, arg);

	if(task->status == CO_STATUS_DEAD) {
		cotask_free(task);
	} else {
		assume(task->status == CO_STATUS_SUSPENDED);
		alist_append(&sched->pending_tasks, task);
	}

	return ret;
}

uint cosched_run_tasks(CoSched *sched) {
	for(CoTask *t; (t = alist_pop(&sched->pending_tasks));)  {
		alist_append(&sched->tasks, t);
	}

	uint ran = 0;

	for(CoTask *t = sched->tasks.first, *next; t; t = next) {
		next = t->next;
		assert(t->status == CO_STATUS_SUSPENDED);
		cotask_resume(t, t->arg);
		++ran;

		if(t->status == CO_STATUS_DEAD) {
			alist_unlink(&sched->tasks, t);
			cotask_free(t);
		}
	}

	return ran;
}

void cosched_free(CoSched *sched) {
	for(CoTask *t = sched->pending_tasks.first, *next; t; t = next) {
		next = t->next;
		cotask_free(t);
	}

	for(CoTask *t = sched->tasks.first, *next; t; t = next) {
		next = t->next;
		cotask_free(t);
	}
}

void coroutines_init(void) {
	memset(&main_task, 0, sizeof(main_task));
	main_task.cothread = co_active();
	main_task.status = CO_STATUS_RUNNING;
	this_task = &main_task;
}
