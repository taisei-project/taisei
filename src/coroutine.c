/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "coroutine.h"
#include "util.h"

#define CO_STACK_SIZE (64 * 1024)

#define TASK_DEBUG
// #define EVT_DEBUG

#ifdef TASK_DEBUG
	#undef TASK_DEBUG
	#define TASK_DEBUG(...) log_debug(__VA_ARGS__)
#else
	#define TASK_DEBUG(...) ((void)0)
#endif

#ifdef EVT_DEBUG
	#undef EVT_DEBUG
	#define EVT_DEBUG(...) log_debug(__VA_ARGS__)
#else
	#define EVT_DEBUG(...) ((void)0)
#endif

struct CoTask {
	LIST_INTERFACE(CoTask);
	List event_subscriber_chain;
	koishi_coroutine_t ko;
	BoxedEntity bound_ent;

	struct {
		CoTaskFunc func;
		void *arg;
	} finalizer;
};

struct CoSched {
	LIST_ANCHOR(CoTask) tasks, pending_tasks;
};

static LIST_ANCHOR(CoTask) task_pool;
static size_t num_tasks_allocated;
static size_t num_tasks_in_use;

CoSched *_cosched_global;

CoTask *cotask_new(CoTaskFunc func) {
	CoTask *task;
	++num_tasks_in_use;

	if((task = alist_pop(&task_pool))) {
		koishi_recycle(&task->ko, func);
		TASK_DEBUG(
			"Recycled task %p for proc %p (%zu tasks allocated / %zu in use)",
			(void*)task, *(void**)&func,
			num_tasks_allocated, num_tasks_in_use
		);
	} else {
		task = calloc(1, sizeof(*task));
		koishi_init(&task->ko, CO_STACK_SIZE, func);
		++num_tasks_allocated;
		TASK_DEBUG(
			"Created new task %p for proc %p (%zu tasks allocated / %zu in use)",
			(void*)task, *(void**)&func,
			num_tasks_allocated, num_tasks_in_use
		);
	}

	return task;
}

void cotask_free(CoTask *task) {
	memset(&task->bound_ent, 0, sizeof(task->bound_ent));
	memset(&task->finalizer, 0, sizeof(task->finalizer));
	alist_push(&task_pool, task);

	--num_tasks_in_use;

	TASK_DEBUG(
		"Released task %p (%zu tasks allocated / %zu in use)",
		(void*)task,
		num_tasks_allocated, num_tasks_in_use
	);
}

CoTask *cotask_active(void) {
	return CASTPTR_ASSUME_ALIGNED((char*)koishi_active() - offsetof(CoTask, ko), CoTask);
}

void *cotask_resume(CoTask *task, void *arg) {
	if(task->bound_ent.ent && !ent_unbox(task->bound_ent)) {
		koishi_kill(&task->ko);

		if(task->finalizer.func) {
			task->finalizer.func(task->finalizer.arg);
		}

		return NULL;
	}

	arg = koishi_resume(&task->ko, arg);

	if(task->finalizer.func && cotask_status(task) == CO_STATUS_DEAD) {
		task->finalizer.func(task->finalizer.arg);
	}

	return arg;
}

void *cotask_yield(void *arg) {
	return koishi_yield(arg);
}

bool cotask_wait_event(CoEvent *evt, void *arg) {
	assert(evt->unique_id > 0);

	struct {
		uint32_t unique_id;
		uint32_t num_signaled;
	} snapshot = { evt->unique_id, evt->num_signaled };

	alist_append(&evt->subscribers, &cotask_active()->event_subscriber_chain);

	while(true) {
		EVT_DEBUG("[%p]", (void*)evt);
		EVT_DEBUG("evt->unique_id        == %u", evt->unique_id);
		EVT_DEBUG("snapshot.unique_id    == %u", snapshot.unique_id);
		EVT_DEBUG("evt->num_signaled     == %u", evt->num_signaled);
		EVT_DEBUG("snapshot.num_signaled == %u", snapshot.num_signaled);

		if(
			evt->unique_id != snapshot.unique_id ||
			evt->num_signaled < snapshot.num_signaled
		) {
			EVT_DEBUG("Event was canceled");
			return false;
		}

		if(evt->num_signaled > snapshot.num_signaled) {
			EVT_DEBUG("Event was signaled");
			return true;
		}

		EVT_DEBUG("Event hasn't changed; waiting...");
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

void cotask_set_finalizer(CoTask *task, CoTaskFunc finalizer, void *arg) {
	assert(task->finalizer.func == NULL);
	task->finalizer.func = finalizer;
	task->finalizer.arg = arg;
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
		TASK_DEBUG("Resume CoEvent{%p} subscriber %p", (void*)evt, (void*)task);
		cotask_resume(task, NULL);
	}

	evt->subscribers.first = evt->subscribers.last = NULL;
}

void coevent_signal(CoEvent *evt) {
	++evt->num_signaled;
	assert(evt->num_signaled != 0);
	coevent_wake_subscribers(evt);
}

void coevent_signal_once(CoEvent *evt) {
	if(!evt->num_signaled) {
		coevent_signal(evt);
	}
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
