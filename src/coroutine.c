/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "coroutine.h"
#include "util.h"

#define CO_STACK_SIZE (128 * 1024)

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
	koishi_coroutine_t ko;
	BoxedEntity bound_ent;

	struct {
		CoTaskFunc func;
		void *arg;
	} finalizer;

	CoTask *supertask;
	ListAnchor subtasks;
	List subtask_chain;

	uint32_t unique_id;
};

#define SUBTASK_NODE_TO_TASK(n) CASTPTR_ASSUME_ALIGNED((char*)(n) - offsetof(CoTask, subtask_chain), CoTask)

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
	task->supertask = NULL;
	memset(&task->bound_ent, 0, sizeof(task->bound_ent));
	memset(&task->finalizer, 0, sizeof(task->finalizer));
	memset(&task->subtasks, 0, sizeof(task->subtasks));
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

static void cotask_finalize(CoTask *task) {
	if(task->finalizer.func) {
		task->finalizer.func(task->finalizer.arg);
		#ifdef DEBUG
		task->finalizer.func = (void*(*)(void*)) 0xDECEA5E;
		#endif
	}

	List *node;

	if(task->supertask) {
		TASK_DEBUG(
			"Slave task %p detaching from master %p",
			(void*)task, (void*)task->supertask
		);

		alist_unlink(&task->supertask->subtasks, &task->subtask_chain);
		task->supertask = NULL;
	}

	while((node = alist_pop(&task->subtasks))) {
		CoTask *sub = SUBTASK_NODE_TO_TASK(node);
		assume(sub->supertask == task);
		sub->supertask = NULL;
		cotask_cancel(sub);

		TASK_DEBUG(
			"Canceled slave task %p (of master task %p)",
			(void*)sub, (void*)task
		);
	}
}

static void cotask_force_cancel(CoTask *task) {
	// WARNING: It's important to finalize first, because if task == active task,
	// then koishi_kill will not return!
	cotask_finalize(task);
	koishi_kill(&task->ko);
	assert(cotask_status(task) == CO_STATUS_DEAD);
}

bool cotask_cancel(CoTask *task) {
	if(cotask_status(task) != CO_STATUS_DEAD) {
		cotask_force_cancel(task);
		return true;
	}

	return false;
}

void *cotask_resume(CoTask *task, void *arg) {
	if(task->bound_ent.ent && !ENT_UNBOX(task->bound_ent)) {
		cotask_force_cancel(task);
		return NULL;
	}

	arg = koishi_resume(&task->ko, arg);

	if(cotask_status(task) == CO_STATUS_DEAD) {
		cotask_finalize(task);
	}

	return arg;
}

void *cotask_yield(void *arg) {
	return koishi_yield(arg);
}

CoWaitResult cotask_wait_event(CoEvent *evt, void *arg) {
	assert(evt->unique_id > 0);

	CoWaitResult result = {
		.frames = 0,
		.event_status = CO_EVENT_PENDING,
	};

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
			result.event_status = CO_EVENT_CANCELED;
			return result;
		}

		if(evt->num_signaled > snapshot.num_signaled) {
			EVT_DEBUG("Event was signaled");
			result.event_status = CO_EVENT_SIGNALED;
			return result;
		}

		EVT_DEBUG("Event hasn't changed; waiting...");
		++result.frames;
		cotask_yield(arg);
	}
}

CoWaitResult cotask_wait_event_or_die(CoEvent *evt, void *arg) {
	CoWaitResult wr = cotask_wait_event(evt, arg);

	if(wr.event_status == CO_EVENT_CANCELED) {
		cotask_cancel(cotask_active());
		UNREACHABLE;
	}

	return wr;
}

CoStatus cotask_status(CoTask *task) {
	return koishi_state(&task->ko);
}

EntityInterface *(cotask_bind_to_entity)(CoTask *task, EntityInterface *ent) {
	assert(task->bound_ent.ent == 0);

	if(ent == NULL) {
		cotask_force_cancel(task);
		UNREACHABLE;
	}

	task->bound_ent = ENT_BOX(ent);
	return ent;
}

void cotask_set_finalizer(CoTask *task, CoTaskFunc finalizer, void *arg) {
	assert(task->finalizer.func == NULL);
	task->finalizer.func = finalizer;
	task->finalizer.arg = arg;
}

void cotask_enslave(CoTask *slave) {
	assert(slave->supertask == NULL);

	CoTask *master = cotask_active();
	assert(master != NULL);

	slave->supertask = master;
	alist_append(&master->subtasks, &slave->subtask_chain);

	TASK_DEBUG(
		"Made task %p a slave of task %p",
		(void*)slave, (void*)slave->supertask
	);
}

void coevent_init(CoEvent *evt) {
	static uint32_t g_uid;
	*evt = (CoEvent) { .unique_id = ++g_uid };
	assert(g_uid != 0);
}

static void coevent_wake_subscribers(CoEvent *evt) {
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
	// FIXME: should we ensure that finalizers get called even here?

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
