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
	List event_subscriber_chain;
	koishi_coroutine_t ko;
	BoxedEntity bound_ent;
};

struct CoSched {
	LIST_ANCHOR(CoTask) tasks, pending_tasks;
};

static LIST_ANCHOR(CoTask) task_pool;
static size_t task_pool_size;

CoSched *_cosched_global;

CoTask *cotask_new(CoTaskFunc func) {
	CoTask *task;

	if((task = alist_pop(&task_pool))) {
		koishi_recycle(&task->ko, func);
		log_debug("Recycled task %p for proc %p", (void*)task, *(void**)&func);
	} else {
		task = calloc(1, sizeof(*task));
		koishi_init(&task->ko, CO_STACK_SIZE, func);
		++task_pool_size;
		log_debug("Created new task %p for proc %p (%zu tasks in pool)", (void*)task, *(void**)&func, task_pool_size);
	}

	return task;
}

void cotask_free(CoTask *task) {
	memset(&task->bound_ent, 0, sizeof(task->bound_ent));
	alist_push(&task_pool, task);
}

CoTask *cotask_active(void) {
	return CASTPTR_ASSUME_ALIGNED((char*)koishi_active() - offsetof(CoTask, ko), CoTask);
}

void *cotask_resume(CoTask *task, void *arg) {
	if(task->bound_ent.ent && !ent_unbox(task->bound_ent)) {
		koishi_kill(&task->ko);
		return NULL;
	}

	return koishi_resume(&task->ko, arg);
}

void *cotask_yield(void *arg) {
	return koishi_yield(arg);
}

bool cotask_wait_event(CoEvent *evt, void *arg) {
	struct {
		uint32_t unique_id;
		uint32_t num_signaled;
	} snapshot = { evt->unique_id, evt->num_signaled };

	alist_append(&evt->subscribers, &cotask_active()->event_subscriber_chain);

	while(true) {
		if(
			evt->unique_id != snapshot.unique_id ||
			evt->num_signaled < snapshot.num_signaled
		) {
			return false;
		}

		if(evt->num_signaled > snapshot.num_signaled) {
			return true;
		}

		cotask_yield(arg);
	}
}

CoStatus cotask_status(CoTask *task) {
	return koishi_state(&task->ko);
}

void cotask_bind_to_entity(CoTask *task, EntityInterface *ent) {
	assert(task->bound_ent.ent == 0);
	task->bound_ent = ent_box(ent);
}

void coevent_init(CoEvent *evt) {
	static uint32_t g_uid;
	evt->unique_id = ++g_uid;
	evt->num_signaled = 0;
	assert(g_uid != 0);
}

static void coevent_wake_subscribers(CoEvent *evt) {
	for(List *node = evt->subscribers.first; node; node = node->next) {
		CoTask *task = CASTPTR_ASSUME_ALIGNED((char*)node - offsetof(CoTask, event_subscriber_chain), CoTask);
		cotask_resume(task, NULL);
	}

	evt->subscribers.first = evt->subscribers.last = NULL;
}

void coevent_signal(CoEvent *evt) {
	++evt->num_signaled;
	assert(evt->num_signaled != 0);
	coevent_wake_subscribers(evt);
}

void coevent_cancel(CoEvent* evt) {
	evt->num_signaled = evt->unique_id = 0;
	coevent_wake_subscribers(evt);
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
		assert(cotask_status(task) == CO_STATUS_SUSPENDED);
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

		if(cotask_status(t) == CO_STATUS_DEAD) {
			alist_unlink(&sched->tasks, t);
			cotask_free(t);
		} else {
			assert(cotask_status(t) == CO_STATUS_SUSPENDED);
			cotask_resume(t, NULL);
			++ran;
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

void coroutines_shutdown(void) {
	for(CoTask *task; (task = alist_pop(&task_pool));) {
		koishi_deinit(&task->ko);
		free(task);
	}
}
