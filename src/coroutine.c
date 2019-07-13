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

#define CO_STACK_SIZE (64 * 1024)

struct CoTask {
	LIST_INTERFACE(CoTask);
	koishi_coroutine_t ko;
};

struct CoSched {
	LIST_ANCHOR(CoTask) tasks, pending_tasks;
};

CoSched *_cosched_global;

CoTask *cotask_new(CoTaskFunc func) {
	CoTask *task = calloc(1, sizeof(*task));
	koishi_init(&task->ko, CO_STACK_SIZE, func);
	return task;
}

void cotask_free(CoTask *task) {
	koishi_deinit(&task->ko);
	free(task);
}

void *cotask_resume(CoTask *task, void *arg) {
	return koishi_resume(&task->ko, arg);
}

void *cotask_yield(void *arg) {
	return koishi_yield(arg);
}

CoStatus cotask_status(CoTask *task) {
	return koishi_state(&task->ko);
}

CoSched *cosched_new(void) {
	CoSched *sched = calloc(1, sizeof(*sched));
	return sched;
}

void *cosched_new_task(CoSched *sched, CoTaskFunc func, void *arg) {
	CoTask *task = cotask_new(func);
	void *ret = cotask_resume(task, arg);

	if(cotask_status(task) == CO_STATUS_DEAD) {
		cotask_free(task);
	} else {
		assume(cotask_status(task) == CO_STATUS_SUSPENDED);
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
		assert(cotask_status(t) == CO_STATUS_SUSPENDED);
		cotask_resume(t, NULL);
		++ran;

		if(cotask_status(t) == CO_STATUS_DEAD) {
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

}
