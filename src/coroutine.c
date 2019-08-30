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

// #define TASK_DEBUG
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
	// List event_subscriber_chain;
	koishi_coroutine_t ko;
	BoxedEntity bound_ent;

	struct {
		CoTaskFunc func;
		void *arg;
	} finalizer;

	uint32_t unique_id;
};

static LIST_ANCHOR(CoTask) task_pool;
static size_t num_tasks_allocated;
static size_t num_tasks_in_use;

CoSched *_cosched_global;

BoxedTask cotask_box(CoTask *task) {
	return (BoxedTask) {
		.ptr = (uintptr_t)task,
		.unique_id = task->unique_id,
	};
}

CoTask *cotask_unbox(BoxedTask box) {
	CoTask *task = (void*)box.ptr;

	if(task->unique_id == box.unique_id) {
		return task;
	}

	return NULL;
}

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

	static uint32_t unique_counter = 0;
	task->unique_id = ++unique_counter;
	assert(unique_counter != 0);

	return task;
}

void cotask_free(CoTask *task) {
	task->unique_id = 0;
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

bool cotask_cancel(CoTask *task) {
	if(cotask_status(task) != CO_STATUS_DEAD) {
		koishi_kill(&task->ko);
		assert(cotask_status(task) == CO_STATUS_DEAD);
		return true;
	}

	return false;
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
		uint16_t num_signaled;
	} snapshot = { evt->unique_id, evt->num_signaled };

	evt->num_subscribers++;
	assert(evt->num_subscribers != 0);

	if(evt->num_subscribers >= evt->num_subscribers_allocated) {
		if(!evt->num_subscribers_allocated) {
			evt->num_subscribers_allocated = 4;
		} else {
			evt->num_subscribers_allocated *= 2;
			assert(evt->num_subscribers_allocated != 0);
		}

		evt->subscribers = realloc(evt->subscribers, sizeof(*evt->subscribers) * evt->num_subscribers_allocated);
	}

	evt->subscribers[evt->num_subscribers - 1] = cotask_box(cotask_active());

	// alist_append(&evt->subscribers, &cotask_active()->event_subscriber_chain);

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

EntityInterface *(cotask_bind_to_entity)(CoTask *task, EntityInterface *ent) {
	assert(task->bound_ent.ent == 0);

	if(ent == NULL) {
		koishi_die(NULL);
	}

	task->bound_ent = ent_box(ent);
	return ent;
}

void cotask_set_finalizer(CoTask *task, CoTaskFunc finalizer, void *arg) {
	assert(task->finalizer.func == NULL);
	task->finalizer.func = finalizer;
	task->finalizer.arg = arg;
}

void coevent_init(CoEvent *evt) {
	static uint32_t g_uid;
	*evt = (CoEvent) { .unique_id = ++g_uid };
	assert(g_uid != 0);
}

static void coevent_wake_subscribers(CoEvent *evt) {
#if 0
	List *subs_head = evt->subscribers.first;
	evt->subscribers.first = evt->subscribers.last = NULL;

	// NOTE: might need to copy the list into an array before iterating, to make sure nothing can corrupt the chain...

	for(List *node = subs_head, *next; node; node = next) {
		next = node->next;
		CoTask *task = CASTPTR_ASSUME_ALIGNED((char*)node - offsetof(CoTask, event_subscriber_chain), CoTask);

		if(cotask_status(task) != CO_STATUS_DEAD) {
			TASK_DEBUG("Resume CoEvent{%p} subscriber %p", (void*)evt, (void*)task);
			cotask_resume(task, NULL);
		}
	}
#endif

	if(!evt->num_subscribers) {
		return;
	}

	assert(evt->num_subscribers);

	BoxedTask subs_snapshot[evt->num_subscribers];
	memcpy(subs_snapshot, evt->subscribers, sizeof(subs_snapshot));
	evt->num_subscribers = 0;

	for(int i = 0; i < ARRAY_SIZE(subs_snapshot); ++i) {
		CoTask *task = cotask_unbox(subs_snapshot[i]);

		if(task && cotask_status(task) != CO_STATUS_DEAD) {
			EVT_DEBUG("Resume CoEvent{%p} subscriber %p", (void*)evt, (void*)task);
			cotask_resume(task, NULL);
		}
	}
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
	evt->num_subscribers_allocated = 0;
	free(evt->subscribers);
	evt->subscribers = NULL;
}

void _coevent_array_action(uint num, CoEvent *events, void (*func)(CoEvent*)) {
	for(uint i = 0; i < num; ++i) {
		func(events + i);
	}
}

void cosched_init(CoSched *sched) {
	memset(sched, 0, sizeof(*sched));
}

CoTask *cosched_new_task(CoSched *sched, CoTaskFunc func, void *arg) {
	CoTask *task = cotask_new(func);
	cotask_resume(task, arg);
	assert(cotask_status(task) == CO_STATUS_SUSPENDED || cotask_status(task) == CO_STATUS_DEAD);
	alist_append(&sched->pending_tasks, task);
	return task;
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

void cosched_finish(CoSched *sched) {
	for(CoTask *t = sched->pending_tasks.first, *next; t; t = next) {
		next = t->next;
		cotask_free(t);
	}

	for(CoTask *t = sched->tasks.first, *next; t; t = next) {
		next = t->next;
		cotask_free(t);
	}

	memset(sched, 0, sizeof(*sched));
}

void coroutines_init(void) {

}

void coroutines_shutdown(void) {
	for(CoTask *task; (task = alist_pop(&task_pool));) {
		koishi_deinit(&task->ko);
		free(task);
	}
}

DEFINE_EXTERN_TASK(_cancel_task_helper) {
	CoTask *task = cotask_unbox(ARGS.task);

	if(task) {
		cotask_cancel(task);
	}
}

#include <projectile.h>
#include <laser.h>
#include <item.h>
#include <enemy.h>
#include <boss.h>
#include <player.h>

#define ENT_TYPE(typename, id) \
	typename *_cotask_bind_to_entity_##typename(CoTask *task, typename *ent) { \
		return ENT_CAST((cotask_bind_to_entity)(task, ent ? &ent->entity_interface : NULL), typename); \
	}

ENT_TYPES
#undef ENT_TYPE
