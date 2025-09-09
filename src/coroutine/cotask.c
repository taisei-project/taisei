/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "coroutine/cotask.h"
#include "coroutine/cotask_internal.h"
#include "coroutine/coevent_internal.h"
#include "log.h"
#include "thread.h"

static CoTaskList task_pool;
static koishi_coroutine_t *co_main;

#ifdef CO_TASK_DEBUG
size_t _cotask_debug_event_id;
#endif

#ifdef CO_TASK_STATS
CoTaskStats cotask_stats;
#endif

#ifdef CO_TASK_STATS_STACK

/*
 * Crude and simple method to estimate stack usage per task: at init time, fill
 * the entire stack with a known 32-bit pattern (the "canary"). The canary is
 * pseudo-random and depends on the task's unique ID. After the task is finished,
 * try to find the first occurrence of the canary since the base of the stack
 * with a fast binary search. If we happen to find a false canary somewhere
 * in the middle of actually used stack space, well, tough luck. This was never
 * meant to be exact, and it's pretty unlikely anyway. A linear search from the
 * end of the region would fix this potential problem, but is slower.
 *
 * Note that we assume that the stack grows down, since that's how it is on most
 * systems.
 */

/*
 * Reserve some space for control structures on the stack; we don't want to
 * overwrite that with canaries. fcontext requires this.
 */
#define STACK_BUFFER_UPPER 64
#define STACK_BUFFER_LOWER 0

// for splitmix32
#include "random.h"

static inline uint32_t get_canary(CoTask *task) {
	uint32_t temp = task->unique_id;
	return splitmix32(&temp);
}

static void *get_stack(CoTask *task, size_t *sz) {
	char *lower = koishi_get_stack(&task->ko, sz);

	// Not all koishi backends support stack inspection. Give up in those cases.
	if(!lower || !*sz) {
		log_debug("koishi_get_stack() returned NULL");
		return NULL;
	}

	char *upper = lower + *sz;
	assert(upper > lower);

	lower += STACK_BUFFER_LOWER;
	upper -= STACK_BUFFER_UPPER;
	*sz = upper - lower;

	// ASan doesn't expect us to access stack memory manually, so we have to dodge false positives.
	ASAN_UNPOISON_MEMORY_REGION(lower, *sz);

	return lower;
}

static void setup_stack(CoTask *task) {
	size_t stack_size;
	void *stack = get_stack(task, &stack_size);

	if(!stack) {
		return;
	}

	uint32_t canary = get_canary(task);
	assert(stack_size == sizeof(canary) * (stack_size / sizeof(canary)));

	for(uint32_t *p = stack; p < (uint32_t*)stack + stack_size / sizeof(canary); ++p) {
		*p = canary;
	}
}

/*
 * Find the first occurrence of a canary since the base of the stack, assuming
 * the base is at the top. Note that since this is essentially a binary search
 * on an array that may or may *not* be sorted, it can sometimes give the wrong
 * answer. This is fine though, since the odds are low and we only care about
 * finding an approximate upper bound on stack usage for all tasks, anyway.
 */
static uint32_t *find_first_canary(uint32_t *region, size_t offset, size_t sz, uint32_t canary) {
	if(sz == 1) {
		return region + offset;
	}

	size_t half_size = sz / 2;
	size_t mid_index = offset + half_size;

	if(region[mid_index] == canary) {
		return find_first_canary(region, offset + half_size, sz - half_size, canary);
	} else {
		return find_first_canary(region, offset, half_size, canary);
	}
}

static void estimate_stack_usage(CoTask *task) {
	size_t stack_size;
	void *stack = get_stack(task, &stack_size);

	if(!stack) {
		return;
	}

	uint32_t canary = get_canary(task);

	size_t num_segments = stack_size / sizeof(canary);
	uint32_t *first_segment = stack;
	uint32_t *p_canary = find_first_canary(stack, 0, num_segments, canary);
	assert(p_canary[1] != canary);

	size_t real_stack_size = stack_size + STACK_BUFFER_LOWER + STACK_BUFFER_UPPER;
	size_t usage = (uintptr_t)(first_segment + num_segments - p_canary) * sizeof(canary) + STACK_BUFFER_UPPER;
	double percentage = usage / (double)real_stack_size;

	if(usage > STAT_VAL(peak_stack_usage)) {
		TASK_DEBUG(">>> %s <<<", task->debug_label);
		log_debug("New peak stack usage: %zu out of %zu (%.02f%%); recommended CO_STACK_SIZE >= %zu",
			usage,
			real_stack_size,
			percentage * 100,
			(size_t)(topow2_u64(usage) * 2)
		);
		STAT_VAL_SET(peak_stack_usage, usage);
	}
}

#else // CO_TASK_STATS_STACK

static void setup_stack(CoTask *task) { }
static void estimate_stack_usage(CoTask *task) { }

#endif // CO_TASK_STATS_STACK


void cotask_global_init(void) {
	co_main = koishi_active();
}

void cotask_global_shutdown(void) {
	for(CoTask *task; (task = alist_pop(&task_pool));) {
		koishi_deinit(&task->ko);
		mem_free(task);
	}
}

attr_nonnull_all attr_returns_nonnull
INLINE CoTask *cotask_from_koishi_coroutine(koishi_coroutine_t *co) {
	return CASTPTR_ASSUME_ALIGNED((char*)co - offsetof(CoTask, ko), CoTask);
}

BoxedTask cotask_box(CoTask *task) {
	return (BoxedTask) {
		.ptr = (uintptr_t)task,
		.unique_id = task->unique_id,
	};
}

CoTask *cotask_unbox(BoxedTask box) {
	CoTask *task = (void*)box.ptr;

	if(task && task->unique_id == box.unique_id) {
		return task;
	}

	return NULL;
}

CoTask *cotask_unbox_notnull(BoxedTask box) {
	CoTask *task = NOT_NULL((void*)box.ptr);

	if(task->unique_id == box.unique_id) {
		return task;
	}

	return NULL;
}

CoTask *cotask_new_internal(koishi_entrypoint_t entry_point) {
	CoTask *task;
	STAT_VAL_ADD(num_tasks_in_use, 1);

	if((task = alist_pop(&task_pool))) {
		koishi_recycle(&task->ko, entry_point);
		TASK_DEBUG(
			"Recycled task %p, entry=%p (%zu tasks allocated / %zu in use)",
			(void*)task, *(void**)&entry_point,
			STAT_VAL(num_tasks_allocated), STAT_VAL(num_tasks_in_use)
		);
	} else {
		task = ALLOC(typeof(*task));
		koishi_init(&task->ko, CO_STACK_SIZE, entry_point);
		STAT_VAL_ADD(num_tasks_allocated, 1);
		TASK_DEBUG(
			"Created new task %p, entry=%p (%zu tasks allocated / %zu in use)",
			(void*)task, *(void**)&entry_point,
			STAT_VAL(num_tasks_allocated), STAT_VAL(num_tasks_in_use)
		);
	}

	static uint32_t unique_counter = 0;
	task->unique_id = ++unique_counter;
	setup_stack(task);
	assert(unique_counter != 0);

	task->data = NULL;

#ifdef CO_TASK_DEBUG
	snprintf(task->debug_label, sizeof(task->debug_label), "<unknown at %p; entry=%p>", (void*)task, *(void**)&entry_point);
#endif

	return task;
}

void *cotask_resume_internal(CoTask *task, void *arg) {
	TASK_DEBUG_EVENT(ev);
	TASK_DEBUG("[%zu] Resuming task %s", ev, task->debug_label);
	STAT_VAL_ADD(num_switches_this_frame, 1);
	arg = koishi_resume(&task->ko, arg);
	TASK_DEBUG("[%zu] koishi_resume returned (%s)", ev, task->debug_label);
	return arg;
}

static void cancel_task_events(CoTaskData *task_data) {
	// HACK: This allows an entity-bound task to wait for its own "finished"
	// event. Can be useful to do some cleanup without spawning a separate task
	// just for that purpose.
	// It's ok to unbind the entity like that, because when we get here, the
	// task is about to die anyway.
	task_data->bound_ent.ent = 0;

	COEVENT_CANCEL_ARRAY(task_data->events);
}

static bool cotask_finalize(CoTask *task) {
	CoTaskData *task_data = cotask_get_data(task);

	TASK_DEBUG_EVENT(ev);

	if(task_data->finalizing) {
		TASK_DEBUG("[%zu] Already finalizing task %s", ev, task->debug_label);
		return false;
	}

	task_data->finalizing = true;

	TASK_DEBUG("[%zu] Finalizing task %s", ev, task->debug_label);
	TASK_DEBUG("[%zu] data = %p", ev, (void*)task_data);

	cancel_task_events(task_data);

	if(task_data->hosted.events) {
		_coevent_array_action(task_data->hosted.num_events, task_data->hosted.events, coevent_cancel);
		task_data->hosted.events = NULL;
		task_data->hosted.num_events = 0;
	}

	if(task_data->hosted.ent) {
		ent_unregister(task_data->hosted.ent);
		task_data->hosted.ent = NULL;
	}

	if(task_data->master) {
		TASK_DEBUG(
			"[%zu] Slave task %s detaching from master %s", ev,
			 task->debug_label, task_data->master->task->debug_label
		);

		alist_unlink(&task_data->master->slaves, task_data);
		task_data->master = NULL;
	}

	if(task_data->wait.wait_type == COTASK_WAIT_EVENT) {
		CoEvent *evt = NOT_NULL(task_data->wait.event.pevent);

		if(evt->unique_id == task_data->wait.event.snapshot.unique_id) {
			coevent_cleanup_subscribers(task_data->wait.event.pevent);
		}
	}

	task_data->wait.wait_type = COTASK_WAIT_NONE;

	attr_unused bool had_slaves = false;

	if(task_data->slaves.first) {
		had_slaves = true;
		TASK_DEBUG("[%zu] Canceling slave tasks for %s...", ev, task->debug_label);
	}

	for(CoTaskData *slave_data; (slave_data = alist_pop(&task_data->slaves));) {
		TASK_DEBUG("[%zu] ... %s", ev, slave_data->task->debug_label);

		assume(slave_data->master == task_data);
		slave_data->master = NULL;
		cotask_cancel(slave_data->task);

		TASK_DEBUG(
			"[%zu] Canceled slave task %s (of master task %s)", ev,
			slave_data->task->debug_label, task->debug_label
		);
	}

	if(had_slaves) {
		TASK_DEBUG("[%zu] DONE canceling slave tasks for %s", ev, task->debug_label);
	}

	CoTaskHeapMemChunk *heap_alloc = task_data->mem.onheap_alloc_head;
	while(heap_alloc) {
		CoTaskHeapMemChunk *next = heap_alloc->next;
		mem_free(heap_alloc);
		heap_alloc = next;
	}

	task->data = NULL;
	TASK_DEBUG("[%zu] DONE finalizing task %s", ev, task->debug_label);

	return true;
}

static void cotask_enslave(CoTaskData *master_data, CoTaskData *slave_data) {
	assert(master_data != NULL);
	assert(slave_data->master == NULL);

	slave_data->master = master_data;
	alist_append(&master_data->slaves, slave_data);

	TASK_DEBUG(
		"Made task %s a slave of task %s",
		slave_data->task->debug_label, master_data->task->debug_label
	);
}

static void cotask_entry_setup(CoTask *task, CoTaskData *data, CoTaskInitData *init_data) {
	task->data = data;
	data->task = task;
	data->sched = init_data->sched;
	data->mem.onstack_alloc_head = data->mem.onstack_alloc_area;

	CoTaskData *master_data = init_data->master_task_data;
	if(master_data) {
		cotask_enslave(master_data, data);
	}

	COEVENT_INIT_ARRAY(data->events);
}

void *cotask_entry(void *varg) {
	CoTaskData data = {};
	CoTaskInitData *init_data = varg;
	CoTask *task = init_data->task;
	cotask_entry_setup(task, &data, init_data);

	varg = init_data->func(init_data->func_arg, init_data->func_arg_size);
	// init_data is now invalid

	TASK_DEBUG("Task %s about to die naturally", task->debug_label);
	coevent_signal(&data.events.finished);
	cotask_finalize(task);

	return varg;
}

void cotask_free(CoTask *task) {
	TASK_DEBUG(
		"Releasing task %s...",
		task->debug_label
	);

	assert(task->data == NULL);

	estimate_stack_usage(task);

	task->unique_id = 0;
	alist_push(&task_pool, task);

	STAT_VAL_ADD(num_tasks_in_use, -1);

	TASK_DEBUG(
		"Released task %s (%zu tasks allocated / %zu in use)",
		task->debug_label,
		STAT_VAL(num_tasks_allocated), STAT_VAL(num_tasks_in_use)
	);
}

CoTask *cotask_active_unsafe(void) {
	koishi_coroutine_t *co = NOT_NULL(koishi_active());
	assert(co != co_main);
	return cotask_from_koishi_coroutine(co);
}

CoTask *cotask_active(void) {
	if(!thread_current_is_main()) {
		// FIXME make this play nice with threads, in case we ever want
		// to use cotasks outside the main thread.
		// Requires a thread-aware build of libkoishi, and making co_main
		// a thread-local variable.
		return NULL;
	}

	koishi_coroutine_t *co = NOT_NULL(koishi_active());

	if(co == co_main || UNLIKELY(co_main == NULL)) {
		return NULL;
	}

	return cotask_from_koishi_coroutine(co);
}

static void cotask_unsafe_cancel(CoTask *task) {
	TASK_DEBUG_EVENT(ev);
	TASK_DEBUG("[%zu] Begin canceling task %s", ev, task->debug_label);

	if(!cotask_finalize(task)) {
		TASK_DEBUG("[%zu] Can't cancel task %s (already finalizing)", ev, task->debug_label);
		return;
	}

	// could have been killed in finalizer indirectly…
	if(cotask_status(task) != CO_STATUS_DEAD) {
		TASK_DEBUG("[%zu] Killing task %s", ev, task->debug_label);
		koishi_kill(&task->ko, NULL);
		TASK_DEBUG("[%zu] koishi_kill returned (%s)", ev, task->debug_label);
		assert(cotask_status(task) == CO_STATUS_DEAD);
	}

	TASK_DEBUG("[%zu] End canceling task %s", ev, task->debug_label);
}

static void *cotask_cancel_in_safe_context(void *arg) {
	CoTask *victim = arg;
	cotask_unsafe_cancel(victim);
	return NULL;
}

static void cotask_force_cancel(CoTask *task) {
	koishi_coroutine_t *ctx = koishi_active();

	if(ctx == &task->ko || ctx == co_main) {
		// It is safe to run the finalizer in the task's own context and in main.
		// In the former case, we prevent a finalizing task from being cancelled
		// while its finalizer is running.
		// In the later case, the main context simply can not be cancelled ever.

		cotask_unsafe_cancel(task);
		// NOTE: this kills task->ko, so it'll only return in case ctx == co_main
		return;
	}

	// Finalizing a task from another task's context is not safe, because the
	// finalizer-executing task may be cancelled by event handlers triggered by the
	// finalizer. The easiest way to get around this is to run the finalizer in
	// another, private context that user code has no reference to, and therefore
	// can't cancel.

	// We'll create a lightweight coroutine for this purpose that does not use
	// CoTaskData, since we don't need any of the 'advanced' features for this.
	// This also means we don't need to cotask_finalize it.

	CoTask *cancel_task = cotask_new_internal(cotask_cancel_in_safe_context);

	// This is basically just koishi_resume + some logging when built with CO_TASK_DEBUG.
	// We can't use normal cotask_resume here, since we don't have CoTaskData.
	cotask_resume_internal(cancel_task, task);

	cotask_free(cancel_task);
}

bool cotask_cancel(CoTask *task) {
	if(!task || cotask_status(task) == CO_STATUS_DEAD) {
		return false;
	}

	cotask_force_cancel(task);
	return true;
}

static void *cotask_force_resume(CoTask *task, void *arg) {
	attr_unused CoTaskData *task_data = cotask_get_data(task);
	assert(task_data->wait.wait_type == COTASK_WAIT_NONE);
	assert(!task_data->bound_ent.ent || ENT_UNBOX(task_data->bound_ent));
	return cotask_resume_internal(task, arg);
}

static void *cotask_wake_and_resume(CoTask *task, void *arg) {
	CoTaskData *task_data = cotask_get_data(task);
	task_data->wait.wait_type = COTASK_WAIT_NONE;
	return cotask_force_resume(task, arg);
}

static bool cotask_do_wait(CoTaskData *task_data) {
	switch(task_data->wait.wait_type) {
		case COTASK_WAIT_NONE: {
			return false;
		}

		case COTASK_WAIT_DELAY: {
			if(--task_data->wait.delay.remaining < 0) {
				return false;
			}

			break;
		}

		case COTASK_WAIT_EVENT: {
			// TASK_DEBUG("COTASK_WAIT_EVENT in task %s", task_data->task->debug_label);

			CoEventStatus stat = coevent_poll(task_data->wait.event.pevent, &task_data->wait.event.snapshot);
			if(stat != CO_EVENT_PENDING) {
				task_data->wait.result.event_status = stat;
				TASK_DEBUG("COTASK_WAIT_EVENT in task %s RESULT = %i", task_data->task->debug_label, stat);
				return false;
			}

			break;
		}

		case COTASK_WAIT_SUBTASKS: {
			if(task_data->slaves.first == NULL) {
				return false;
			}

			break;
		}
	}

	task_data->wait.result.frames++;
	return true;
}

void *cotask_resume(CoTask *task, void *arg) {
	CoTaskData *task_data = cotask_get_data(task);

	if(task_data->bound_ent.ent && !ENT_UNBOX(task_data->bound_ent)) {
		cotask_force_cancel(task);
		return NULL;
	}

	if(!cotask_do_wait(task_data)) {
		return cotask_wake_and_resume(task, arg);
	}

	assert(task_data->wait.wait_type != COTASK_WAIT_NONE);
	return NULL;
}

void *cotask_yield(void *arg) {
#ifdef CO_TASK_DEBUG
	CoTask *task = cotask_active();
#endif
	TASK_DEBUG_EVENT(ev);
	// TASK_DEBUG("[%zu] Yielding from task %s", ev, task->debug_label);
	STAT_VAL_ADD(num_switches_this_frame, 1);
	arg = koishi_yield(arg);
	// TASK_DEBUG("[%zu] koishi_yield returned (%s)", ev, task->debug_label);
	return arg;
}

static inline CoWaitResult cotask_wait_init(CoTaskData *task_data, char wait_type) {
	CoWaitResult wr = task_data->wait.result;
	task_data->wait = (typeof(task_data->wait)) {
		.wait_type = wait_type,
	};
	return wr;
}

int cotask_wait(int delay) {
	CoTask *task = cotask_active_unsafe();
	CoTaskData *task_data = cotask_get_data(task);
	assert(task_data->wait.wait_type == COTASK_WAIT_NONE);

	if(delay == 1) {
		cotask_yield(NULL);
		return 1;
	}

	cotask_wait_init(task_data, COTASK_WAIT_DELAY);
	task_data->wait.delay.remaining = delay;

	if(cotask_do_wait(task_data)) {
		cotask_yield(NULL);
	}

	return cotask_wait_init(task_data, COTASK_WAIT_NONE).frames;
}

int cotask_wait_subtasks(void) {
	CoTask *task = cotask_active_unsafe();
	CoTaskData *task_data = cotask_get_data(task);
	assert(task_data->wait.wait_type == COTASK_WAIT_NONE);

	cotask_wait_init(task_data, COTASK_WAIT_SUBTASKS);

	if(cotask_do_wait(task_data)) {
		cotask_yield(NULL);
	}

	return cotask_wait_init(task_data, COTASK_WAIT_NONE).frames;
}

static void *_cotask_malloc(CoTaskData *task_data, size_t size, bool allow_heap_fallback) {
	assert(size > 0);
	assert(size < PTRDIFF_MAX);

	void *mem = NULL;
	ptrdiff_t available_on_stack = (task_data->mem.onstack_alloc_area + sizeof(task_data->mem.onstack_alloc_area)) - task_data->mem.onstack_alloc_head;

	if(available_on_stack >= (ptrdiff_t)size) {
		log_debug("Requested size=%zu, available=%zi, serving from the stack", size, (ssize_t)available_on_stack);
		mem = task_data->mem.onstack_alloc_head;
		task_data->mem.onstack_alloc_head += MEM_ALIGN_SIZE(size);
	} else {
		if(!allow_heap_fallback) {
			UNREACHABLE;
		}

		log_warn("Requested size=%zu, available=%zi, serving from the heap", size, (ssize_t)available_on_stack);
		auto chunk = ALLOC_FLEX(CoTaskHeapMemChunk, size);
		chunk->next = task_data->mem.onheap_alloc_head;
		task_data->mem.onheap_alloc_head = chunk;
		mem = chunk->data;
	}

	return ASSUME_ALIGNED(mem, MEM_ALLOC_ALIGNMENT);
}

void *cotask_malloc(CoTask *task, size_t size) {
	CoTaskData *task_data = cotask_get_data(task);
	return _cotask_malloc(task_data, size, true);
}

EntityInterface *cotask_host_entity(CoTask *task, size_t ent_size, EntityType ent_type) {
	CoTaskData *task_data = cotask_get_data(task);
	assume(task_data->hosted.ent == NULL);
	EntityInterface *ent = _cotask_malloc(task_data, ent_size, false);
	ent_register(ent, ent_type);
	task_data->hosted.ent = ent;
	return ent;
}

void cotask_host_events(CoTask *task, uint num_events, CoEvent events[num_events]) {
	CoTaskData *task_data = cotask_get_data(task);
	assume(task_data->hosted.events == NULL);
	assume(task_data->hosted.num_events == 0);
	assume(num_events > 0);
	task_data->hosted.events = events;
	task_data->hosted.num_events = num_events;
	_coevent_array_action(num_events, events, coevent_init);
}

static CoWaitResult cotask_wait_event_internal(CoEvent *evt, bool once) {
	CoTask *task = cotask_active_unsafe();
	CoTaskData *task_data = cotask_get_data(task);

	// Edge case: if the task is being finalized, don't try to subscribe to another event.
	// This deals with some cases of WAIT_EVENT() deadlocking in loops.
	// cotask_yield() should land us somewhere inside cotask_finalize(), and the task will not be
	// resumed again.
	if(UNLIKELY(task_data->finalizing)) {
		cotask_yield(NULL);
		UNREACHABLE;
	}

	if(evt->unique_id == 0) {
		return (CoWaitResult) { .event_status = CO_EVENT_CANCELED };
	}

	if(once && evt->num_signaled > 0) {
		return (CoWaitResult) { .event_status = CO_EVENT_SIGNALED };
	}

	coevent_add_subscriber(evt, task);

	cotask_wait_init(task_data, COTASK_WAIT_EVENT);
	task_data->wait.event.pevent = evt;
	task_data->wait.event.snapshot = coevent_snapshot(evt);

	if(cotask_do_wait(task_data)) {
		cotask_yield(NULL);
	}

	return cotask_wait_init(task_data, COTASK_WAIT_NONE);
}

CoWaitResult cotask_wait_event(CoEvent *evt) {
	return cotask_wait_event_internal(evt, false);
}

CoWaitResult cotask_wait_event_once(CoEvent *evt) {
	return cotask_wait_event_internal(evt, true);
}

CoWaitResult cotask_wait_event_or_die(CoEvent *evt) {
	CoWaitResult wr = cotask_wait_event(evt);

	if(wr.event_status == CO_EVENT_CANCELED) {
		cotask_cancel(cotask_active_unsafe());
		// Edge case: should usually die here, unless the task has already been cancelled and is
		// currently being finalized. In that case, we should just yield back to cotask_finalize().
		assert(cotask_active_unsafe()->data->finalizing);
		cotask_yield(NULL);
		UNREACHABLE;
	}

	return wr;
}

CoStatus cotask_status(CoTask *task) {
	return koishi_state(&task->ko);
}

CoSched *cotask_get_sched(CoTask *task) {
	return cotask_get_data(task)->sched;
}

EntityInterface *(cotask_bind_to_entity)(CoTask *task, EntityInterface *ent) {
	CoTaskData *task_data = cotask_get_data(task);
	assert(task_data->bound_ent.ent == 0);

	if(ent == NULL) {
		cotask_force_cancel(task);
		UNREACHABLE;
	}

	task_data->bound_ent = ENT_BOX(ent);
	return ent;
}

CoTaskEvents *cotask_get_events(CoTask *task) {
	CoTaskData *task_data = cotask_get_data(task);
	return &task_data->events;
}

void cotask_force_finish(CoTask *task) {
	TASK_DEBUG("Finishing task %s", task->debug_label);

	if(task->data) {
		TASK_DEBUG("Task %s has data, finalizing", task->debug_label);
		cotask_finalize(task);
	} else {
		TASK_DEBUG("Task %s had no data", task->debug_label);
	}

	cotask_free(task);
}

const char *cotask_get_name(CoTask *task) {
	return task->name;
}
