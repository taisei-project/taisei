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

// TODO refactor this intro a few smaller files under coroutine/

#ifdef ADDRESS_SANITIZER
	#include <sanitizer/asan_interface.h>
#else
	#define ASAN_UNPOISON_MEMORY_REGION(addr, size) ((void)0)
#endif

#ifdef __EMSCRIPTEN__
	#define CO_STACK_SIZE (64 * 1024)
#else
	#define CO_STACK_SIZE (256 * 1024)
#endif

// #define EVT_DEBUG

#ifdef DEBUG
	#define CO_TASK_STATS
#endif

#ifdef CO_TASK_DEBUG
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

enum {
	COTASK_WAIT_NONE,
	COTASK_WAIT_DELAY,
	COTASK_WAIT_EVENT,
};

#define MEM_AREA_SIZE (1 << 12)
#define MEM_ALLOC_ALIGNMENT alignof(max_align_t)
#define MEM_ALIGN_SIZE(x) (x + (MEM_ALLOC_ALIGNMENT - 1)) & ~(MEM_ALLOC_ALIGNMENT - 1)

typedef struct CoTaskData CoTaskData;

struct CoTask {
	LIST_INTERFACE(CoTask);
	koishi_coroutine_t ko;

	// Pointer to a control structure on the coroutine's stack
	CoTaskData *data;

	uint32_t unique_id;

#ifdef CO_TASK_DEBUG
	char debug_label[256];
#endif
};

typedef struct CoTaskHeapMemChunk {
	struct CoTaskHeapMemChunk *next;
	alignas(MEM_ALLOC_ALIGNMENT) char data[];
} CoTaskHeapMemChunk;

struct CoTaskData {
	LIST_INTERFACE(CoTaskData);

	CoTask *task;

	CoTaskData *master;              // AKA supertask
	LIST_ANCHOR(CoTaskData) slaves;  // AKA subtasks

	BoxedEntity bound_ent;

	struct {
		CoWaitResult result;
		char wait_type;

		union {
			struct {
				int remaining;
			} delay;

			struct {
				CoEvent *pevent;
				CoEventSnapshot snapshot;
			} event;
		};
	} wait;

	struct {
		EntityInterface *ent;
		CoEvent *events;
		uint num_events;
	} hosted;

	CoTaskEvents events;

	struct {
		CoTaskHeapMemChunk *onheap_alloc_head;
		char *onstack_alloc_head;
		alignas(MEM_ALLOC_ALIGNMENT) char onstack_alloc_area[MEM_AREA_SIZE];
	} mem;
};

typedef struct CoTaskInitData {
	CoTask *task;
	CoTaskFunc func;
	void *func_arg;
	CoTaskData *master_task_data;
} CoTaskInitData;

static LIST_ANCHOR(CoTask) task_pool;

CoSched *_cosched_global;

#ifdef CO_TASK_STATS
static struct {
	size_t num_tasks_allocated;
	size_t num_tasks_in_use;
	size_t num_switches_this_frame;
	size_t peak_stack_usage;
} cotask_stats;

#define STAT_VAL(name) (cotask_stats.name)
#define STAT_VAL_SET(name, value) ((cotask_stats.name) = (value))

// enable stack usage tracking (loose)
#ifndef _WIN32
// NOTE: disabled by default because of heavy performance overhead under ASan
// #define CO_TASK_STATS_STACK
#endif

#else // CO_TASK_STATS

#define STAT_VAL(name) ((void)0)
#define STAT_VAL_SET(name, value) ((void)0)

#endif // CO_TASK_STATS

#define STAT_VAL_ADD(name, value) STAT_VAL_SET(name, STAT_VAL(name) + (value))

#ifdef CO_TASK_DEBUG
static size_t debug_event_id;
#define TASK_DEBUG_EVENT(ev) uint64_t ev = debug_event_id++
#else
#define TASK_DEBUG_EVENT(ev) ((void)0)
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

static CoTask *cotask_new_internal(CoTaskFunc entry_point) {
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
		task = calloc(1, sizeof(*task));
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

static void *cotask_resume_internal(CoTask *task, void *arg) {
	TASK_DEBUG_EVENT(ev);
	TASK_DEBUG("[%zu] Resuming task %s", ev, task->debug_label);
	STAT_VAL_ADD(num_switches_this_frame, 1);
	arg = koishi_resume(&task->ko, arg);
	TASK_DEBUG("[%zu] koishi_resume returned (%s)", ev, task->debug_label);
	return arg;
}

attr_returns_nonnull attr_nonnull_all
INLINE CoTaskData *get_task_data(CoTask *task) {
	CoTaskData *data = task->data;
	assume(data != NULL);
	assume(data->task == task);
	return data;
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

static void coevent_cleanup_subscribers(CoEvent *evt);

static void cotask_finalize(CoTask *task) {
	CoTaskData *task_data = get_task_data(task);
	TASK_DEBUG("Finalizing task %s", task->debug_label);
	TASK_DEBUG("data = %p", (void*)task_data);

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

	if(task_data->wait.wait_type == COTASK_WAIT_EVENT) {
		CoEvent *evt = NOT_NULL(task_data->wait.event.pevent);

		if(evt->unique_id == task_data->wait.event.snapshot.unique_id) {
			coevent_cleanup_subscribers(task_data->wait.event.pevent);
		}
	}

	if(task_data->master) {
		TASK_DEBUG(
			"Slave task %s detaching from master %s",
			 task->debug_label, task_data->master->task->debug_label
		);

		alist_unlink(&task_data->master->slaves, task_data);
		task_data->master = NULL;
	}

	attr_unused bool had_slaves = false;

	if(task_data->slaves.first) {
		had_slaves = true;
		TASK_DEBUG("Canceling slave tasks for %s...", task->debug_label);
	}

	for(CoTaskData *slave_data; (slave_data = alist_pop(&task_data->slaves));) {
		TASK_DEBUG("... %s", slave_data->task->debug_label);

		assume(slave_data->master == task_data);
		slave_data->master = NULL;
		cotask_cancel(slave_data->task);

		TASK_DEBUG(
			"Canceled slave task %s (of master task %s)",
			slave_data->task->debug_label, task->debug_label
		);
	}

	if(had_slaves) {
		TASK_DEBUG("DONE canceling slave tasks for %s", task->debug_label);
	}

	CoTaskHeapMemChunk *heap_alloc = task_data->mem.onheap_alloc_head;
	while(heap_alloc) {
		CoTaskHeapMemChunk *next = heap_alloc->next;
		free(heap_alloc);
		heap_alloc = next;
	}

	task->data = NULL;
	TASK_DEBUG("DONE finalizing task %s", task->debug_label);
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

static void cotask_finalize(CoTask *task);

static void cotask_entry_setup(CoTask *task, CoTaskData *data, CoTaskInitData *init_data) {
	task->data = data;
	data->task = task;

	data->mem.onstack_alloc_head = data->mem.onstack_alloc_area;

	CoTaskData *master_data = init_data->master_task_data;
	if(master_data) {
		cotask_enslave(master_data, data);
	}

	COEVENT_INIT_ARRAY(data->events);
}

static void *cotask_entry(void *varg) {
	CoTaskData data = { 0 };
	CoTaskInitData *init_data = varg;
	CoTask *task = init_data->task;
	cotask_entry_setup(task, &data, init_data);
	CoTaskFunc func = init_data->func;

	varg = cotask_yield(NULL);
	// init_data is now invalid

	varg = func(varg);

	TASK_DEBUG("Task %s about to die naturally", task->debug_label);
	coevent_signal(&data.events.finished);
	cotask_finalize(task);

	return varg;
}

static void *cotask_entry_noyield(void *varg) {
	CoTaskData data = { 0 };
	CoTaskInitData *init_data = varg;
	CoTask *task = init_data->task;
	cotask_entry_setup(task, &data, init_data);

	varg = init_data->func(init_data->func_arg);
	// init_data is now invalid

	TASK_DEBUG("Task %s about to die naturally", task->debug_label);
	coevent_signal(&data.events.finished);
	cotask_finalize(task);

	return varg;
}

CoTask *cotask_new(CoTaskFunc func) {
	CoTask *task = cotask_new_internal(cotask_entry);
	CoTaskInitData init_data = { 0 };
	init_data.task = task;
	init_data.func = func;
	cotask_resume_internal(task, &init_data);
	assert(task->data != NULL);
	return task;
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

CoTask *cotask_active(void) {
	return CASTPTR_ASSUME_ALIGNED((char*)koishi_active() - offsetof(CoTask, ko), CoTask);
}

static void cotask_force_cancel(CoTask *task) {
	// WARNING: It's important to finalize first, because if task == active task,
	// then koishi_kill will not return!
	TASK_DEBUG_EVENT(ev);
	TASK_DEBUG("[%zu] Begin canceling task %s", ev, task->debug_label);
	cotask_finalize(task);
	TASK_DEBUG("[%zu] Killing task %s", ev, task->debug_label);
	koishi_kill(&task->ko, NULL);
	TASK_DEBUG("[%zu] koishi_kill returned (%s)", ev, task->debug_label);
	assert(cotask_status(task) == CO_STATUS_DEAD);
}

bool cotask_cancel(CoTask *task) {
	if(cotask_status(task) != CO_STATUS_DEAD) {
		cotask_force_cancel(task);
		return true;
	}

	return false;
}

static void *cotask_force_resume(CoTask *task, void *arg) {
	attr_unused CoTaskData *task_data = get_task_data(task);
	assert(task_data->wait.wait_type == COTASK_WAIT_NONE);
	assert(!task_data->bound_ent.ent || ENT_UNBOX(task_data->bound_ent));
	return cotask_resume_internal(task, arg);
}

static void *cotask_wake_and_resume(CoTask *task, void *arg) {
	CoTaskData *task_data = get_task_data(task);
	task_data->wait.wait_type = COTASK_WAIT_NONE;
	return cotask_force_resume(task, arg);
}

CoEventSnapshot coevent_snapshot(const CoEvent *evt) {
	return (CoEventSnapshot) {
		.unique_id = evt->unique_id,
		.num_signaled = evt->num_signaled,
	};
}

CoEventStatus coevent_poll(const CoEvent *evt, const CoEventSnapshot *snap) {
#if 0
	EVT_DEBUG("[%p]", (void*)evt);
	EVT_DEBUG("evt->unique_id     == %u", evt->unique_id);
	EVT_DEBUG("snap->unique_id    == %u", snap->unique_id);
	EVT_DEBUG("evt->num_signaled  == %u", evt->num_signaled);
	EVT_DEBUG("snap->num_signaled == %u", snap->num_signaled);
#endif

	if(
		evt->unique_id != snap->unique_id ||
		evt->num_signaled < snap->num_signaled ||
		evt->unique_id == 0
	) {
		EVT_DEBUG("[%p / %u] Event was canceled", (void*)evt, evt->unique_id);
		return CO_EVENT_CANCELED;
	}

	if(evt->num_signaled > snap->num_signaled) {
		EVT_DEBUG("[%p / %u] Event was signaled", (void*)evt, evt->unique_id);
		return CO_EVENT_SIGNALED;
	}

	// EVT_DEBUG("Event hasn't changed; waiting...");
	return CO_EVENT_PENDING;
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
	}

	task_data->wait.result.frames++;
	return true;
}

void *cotask_resume(CoTask *task, void *arg) {
	CoTaskData *task_data = get_task_data(task);

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
	memset(&task_data->wait, 0, sizeof(task_data->wait));
	task_data->wait.wait_type = wait_type;
	return wr;
}

int cotask_wait(int delay) {
	CoTask *task = cotask_active();
	CoTaskData *task_data = get_task_data(task);
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
		CoTaskHeapMemChunk *chunk = calloc(1, sizeof(*chunk) + size);
		chunk->next = task_data->mem.onheap_alloc_head;
		task_data->mem.onheap_alloc_head = chunk;
		mem = chunk->data;
	}

	return ASSUME_ALIGNED(mem, MEM_ALLOC_ALIGNMENT);
}

void *cotask_malloc(CoTask *task, size_t size) {
	CoTaskData *task_data = get_task_data(task);
	return _cotask_malloc(task_data, size, true);
}

EntityInterface *cotask_host_entity(CoTask *task, size_t ent_size, EntityType ent_type) {
	CoTaskData *task_data = get_task_data(task);
	assume(task_data->hosted.ent == NULL);
	EntityInterface *ent = _cotask_malloc(task_data, ent_size, false);
	ent_register(ent, ent_type);
	task_data->hosted.ent = ent;
	return ent;
}

void cotask_host_events(CoTask *task, uint num_events, CoEvent events[num_events]) {
	CoTaskData *task_data = get_task_data(task);
	assume(task_data->hosted.events == NULL);
	assume(task_data->hosted.num_events == 0);
	assume(num_events > 0);
	task_data->hosted.events = events;
	task_data->hosted.num_events = num_events;
	_coevent_array_action(num_events, events, coevent_init);
}

static bool subscribers_array_predicate(const void *pelem, void *userdata) {
	return cotask_unbox(*(const BoxedTask*)pelem);
}

static void coevent_cleanup_subscribers(CoEvent *evt) {
	if(evt->subscribers.num_elements == 0) {
		return;
	}

	attr_unused uint prev_num_subs = evt->subscribers.num_elements;
	dynarray_filter(&evt->subscribers, subscribers_array_predicate, NULL);
	attr_unused uint new_num_subs = evt->subscribers.num_elements;
	EVT_DEBUG("Event %p num subscribers %u -> %u", (void*)evt, prev_num_subs, new_num_subs);
}

static void coevent_add_subscriber(CoEvent *evt, CoTask *task) {
	EVT_DEBUG("Event %p (num=%u; capacity=%u)", (void*)evt, evt->subscribers.num_elements, evt->subscribers.capacity);
	EVT_DEBUG("Subscriber: %s", task->debug_label);

	*dynarray_append_with_min_capacity(&evt->subscribers, 4) = cotask_box(task);
}

static CoWaitResult cotask_wait_event_internal(CoEvent *evt) {
	CoTask *task = cotask_active();
	CoTaskData *task_data = get_task_data(task);

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
	if(evt->unique_id == 0) {
		return (CoWaitResult) { .event_status = CO_EVENT_CANCELED };
	}

	return cotask_wait_event_internal(evt);
}

CoWaitResult cotask_wait_event_once(CoEvent *evt) {
	if(evt->unique_id == 0) {
		return (CoWaitResult) { .event_status = CO_EVENT_CANCELED };
	}

	if(evt->num_signaled > 0) {
		return (CoWaitResult) { .event_status = CO_EVENT_SIGNALED };
	}

	return cotask_wait_event_internal(evt);
}

CoWaitResult cotask_wait_event_or_die(CoEvent *evt) {
	CoWaitResult wr = cotask_wait_event(evt);

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
	CoTaskData *task_data = get_task_data(task);
	assert(task_data->bound_ent.ent == 0);

	if(ent == NULL) {
		cotask_force_cancel(task);
		UNREACHABLE;
	}

	task_data->bound_ent = ENT_BOX(ent);
	return ent;
}

CoTaskEvents *cotask_get_events(CoTask *task) {
	CoTaskData *task_data = get_task_data(task);
	return &task_data->events;
}

void coevent_init(CoEvent *evt) {
	static uint32_t g_uid;
	uint32_t uid = ++g_uid;
	EVT_DEBUG("Init event %p (uid = %u)", (void*)evt, uid);
	*evt = (CoEvent) { .unique_id = uid };
	assert(g_uid != 0);
}

static void coevent_wake_subscribers(CoEvent *evt, uint num_subs, BoxedTask subs[num_subs]) {
	for(int i = 0; i < num_subs; ++i) {
		CoTask *task = cotask_unbox(subs[i]);

		if(task && cotask_status(task) != CO_STATUS_DEAD) {
			EVT_DEBUG("Resume CoEvent{%p} subscriber %s", (void*)evt, task->debug_label);
			cotask_resume(task, NULL);
		}
	}
}

void coevent_signal(CoEvent *evt) {
	if(UNLIKELY(evt->unique_id == 0)) {
		return;
	}

	++evt->num_signaled;
	EVT_DEBUG("Signal event %p (uid = %u; num_signaled = %u)", (void*)evt, evt->unique_id, evt->num_signaled);
	assert(evt->num_signaled != 0);

	if(evt->subscribers.num_elements) {
		BoxedTask subs_snapshot[evt->subscribers.num_elements];
		memcpy(subs_snapshot, evt->subscribers.data, sizeof(subs_snapshot));
		evt->subscribers.num_elements = 0;
		coevent_wake_subscribers(evt, ARRAY_SIZE(subs_snapshot), subs_snapshot);
	}
}

void coevent_signal_once(CoEvent *evt) {
	if(!evt->num_signaled) {
		coevent_signal(evt);
	}
}

void coevent_cancel(CoEvent *evt) {
	TASK_DEBUG_EVENT(ev);

	if(evt->unique_id == 0) {
		EVT_DEBUG("[%lu] Event %p already canceled", ev, (void*)evt);
		return;
	}

	EVT_DEBUG("[%lu] BEGIN Cancel event %p (uid = %u; num_signaled = %u)", ev,  (void*)evt, evt->unique_id, evt->num_signaled);
	EVT_DEBUG("[%lu] SUBS = %p", ev,  (void*)evt->subscribers.data);
	evt->unique_id = 0;

	if(evt->subscribers.num_elements) {
		BoxedTask subs_snapshot[evt->subscribers.num_elements];
		memcpy(subs_snapshot, evt->subscribers.data, sizeof(subs_snapshot));
		dynarray_free_data(&evt->subscribers);
		coevent_wake_subscribers(evt, ARRAY_SIZE(subs_snapshot), subs_snapshot);
		// CAUTION: no modifying evt after this point, it may be invalidated
	} else {
		dynarray_free_data(&evt->subscribers);
	}

	EVT_DEBUG("[%lu] END Cancel event %p", ev, (void*)evt);
}

void _coevent_array_action(uint num, CoEvent *events, void (*func)(CoEvent*)) {
	for(uint i = 0; i < num; ++i) {
		func(events + i);
	}
}

void cosched_init(CoSched *sched) {
	memset(sched, 0, sizeof(*sched));
}

CoTask *_cosched_new_task(CoSched *sched, CoTaskFunc func, void *arg, bool is_subtask, CoTaskDebugInfo debug) {
	CoTask *task = cotask_new_internal(cotask_entry_noyield);

#ifdef CO_TASK_DEBUG
	snprintf(task->debug_label, sizeof(task->debug_label), "#%i <%p> %s (%s:%i:%s)", task->unique_id, (void*)task, debug.label, debug.debug_info.file, debug.debug_info.line, debug.debug_info.func);
#endif

	CoTaskInitData init_data = { 0 };
	init_data.task = task;
	init_data.func = func;
	init_data.func_arg = arg;

	if(is_subtask) {
		init_data.master_task_data = get_task_data(cotask_active());
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

static void force_finish_task(CoTask *task) {
	TASK_DEBUG("Finishing task %s", task->debug_label);

	if(task->data) {
		TASK_DEBUG("Task %s has data, finalizing", task->debug_label);
		cotask_finalize(task);
	} else {
		TASK_DEBUG("Task %s had no data", task->debug_label);
	}

	cotask_free(task);
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
		force_finish_task(t);
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

#ifdef CO_TASK_STATS
#include "video.h"
#include "resource/font.h"
#endif

void coroutines_draw_stats(void) {
#ifdef CO_TASK_STATS
	if(STAT_VAL(num_tasks_in_use) == 0 && STAT_VAL(num_switches_this_frame) == 0) {
		return;
	}

	static char buf[128];

	TextParams tp = {
		.pos = { SCREEN_W },
		.color = RGB(1, 1, 1),
		.shader_ptr = res_shader("text_default"),
		.font_ptr = res_font("monotiny"),
		.align = ALIGN_RIGHT,
	};

	float ls = font_get_lineskip(tp.font_ptr);

	tp.pos.y += ls;

#ifdef CO_TASK_STATS_STACK
	snprintf(buf, sizeof(buf), "Peak stack: %zukb    Tasks: %4zu / %4zu ",
		STAT_VAL(peak_stack_usage) / 1024,
		STAT_VAL(num_tasks_in_use),
		STAT_VAL(num_tasks_allocated)
	);
#else
	snprintf(buf, sizeof(buf), "Tasks: %4zu / %4zu ",
		STAT_VAL(num_tasks_in_use),
		STAT_VAL(num_tasks_allocated)
	);
#endif

	text_draw(buf, &tp);

	tp.pos.y += ls;
	snprintf(buf, sizeof(buf), "Switches/frame: %4zu ", STAT_VAL(num_switches_this_frame));
	text_draw(buf, &tp);

	STAT_VAL_SET(num_switches_this_frame, 0);
#endif
}

DEFINE_EXTERN_TASK(_cancel_task_helper) {
	CoTask *task = cotask_unbox(ARGS.task);

	if(task) {
		cotask_cancel(task);
	}
}
