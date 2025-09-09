/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "coroutine/cosched.h"
#include "coroutine/cotask.h"
#include "coroutine/cotask_internal.h"
#include "hashtable.h"

void cosched_init(CoSched *sched) {
	*sched = (typeof(*sched)) {};
}

CoTask *_cosched_new_task(CoSched *sched, CoTaskFunc func, void *arg, size_t arg_size, bool is_subtask, CoTaskDebugInfo debug) {
	assume(sched != NULL);
	CoTask *task = cotask_new_internal(cotask_entry);
	task->name = debug.label;

#ifdef CO_TASK_DEBUG
	snprintf(task->debug_label, sizeof(task->debug_label), "#%i <%p> %s (%s:%i:%s)", task->unique_id, (void*)task, debug.label, debug.debug_info.file, debug.debug_info.line, debug.debug_info.func);
#endif

	CoTaskInitData init_data = {};
	init_data.task = task;
	init_data.sched = sched;
	init_data.func = func;
	init_data.func_arg = arg;
	init_data.func_arg_size = arg_size;

	if(is_subtask) {
		init_data.master_task_data = cotask_get_data(cotask_active_unsafe());
		assert(init_data.master_task_data != NULL);
	}

	alist_append(&sched->pending_tasks, task);
	cotask_resume_internal(task, &init_data);

	assert(cotask_status(task) == CO_STATUS_SUSPENDED || cotask_status(task) == CO_STATUS_DEAD);

	return task;
}

uint cosched_run_tasks(CoSched *sched) {
	alist_merge_tail(&sched->tasks, &sched->pending_tasks);

	uint ran = 0;

	TASK_DEBUG("---------------------------------------------------------------");
	for(CoTask *t = sched->tasks.first, *next; t; t = next) {
		next = t->next;

		if(cotask_status(t) == CO_STATUS_DEAD) {
			TASK_DEBUG("<!> %s", t->debug_label);
			alist_unlink(&sched->tasks, t);
			cotask_free(t);
		} else {
			TASK_DEBUG(">>> %s", t->debug_label);
			assert(cotask_status(t) == CO_STATUS_SUSPENDED);
			cotask_resume(t, NULL);
			++ran;
		}
	}
	TASK_DEBUG("---------------------------------------------------------------");

	return ran;
}

typedef ht_ptr2int_t events_hashset;

static uint gather_blocking_events(CoTaskList *tasks, events_hashset *events) {
	uint n = 0;

	for(CoTask *t = tasks->first; t; t = t->next) {
		if(!t->data) {
			continue;
		}

		CoTaskData *tdata = t->data;

		if(tdata->wait.wait_type != COTASK_WAIT_EVENT) {
			continue;
		}

		CoEvent *e = tdata->wait.event.pevent;

		if(e->unique_id != tdata->wait.event.snapshot.unique_id) {
			// event not valid? (probably should not happen)
			continue;
		}

		ht_set(events, e, e->unique_id);
		++n;
	}

	return n;
}

static void cancel_blocking_events(CoSched *sched) {
	events_hashset events;
	ht_create(&events);

	gather_blocking_events(&sched->tasks, &events);
	gather_blocking_events(&sched->pending_tasks, &events);

	ht_ptr2int_iter_t iter;
	ht_iter_begin(&events, &iter);

	for(;iter.has_data; ht_iter_next(&iter)) {
		CoEvent *e = iter.key;
		if(e->unique_id == iter.value) {
			// NOTE: wakes subscribers, which may cancel/invalidate other events before we do.
			// This is why we snapshot unique_id.
			// We assume that the memory backing *e is safe to access, however.
			coevent_cancel(e);
		}
	}

	ht_destroy(&events);
}

static void finish_task_list(CoTaskList *tasks) {
	for(CoTask *t; (t = alist_pop(tasks));) {
		cotask_force_finish(t);
	}
}

void cosched_finish(CoSched *sched) {
	// First cancel all events that have any tasks waiting on them.
	// This will wake those tasks, so they can do any necessary cleanup.
	cancel_blocking_events(sched);
	finish_task_list(&sched->tasks);
	finish_task_list(&sched->pending_tasks);
	assert(!sched->tasks.first);
	assert(!sched->pending_tasks.first);
	*sched = (typeof(*sched)) {};
}
