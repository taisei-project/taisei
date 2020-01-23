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

#ifdef ADDRESS_SANITIZER
	#include <sanitizer/asan_interface.h>
#else
	#define ASAN_UNPOISON_MEMORY_REGION(addr, size) ((void)0)
#endif

#define CO_STACK_SIZE (64 * 1024)

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

#ifdef CO_TASK_DEBUG
	char debug_label[256];
#endif
};

#define SUBTASK_NODE_TO_TASK(n) CASTPTR_ASSUME_ALIGNED((char*)(n) - offsetof(CoTask, subtask_chain), CoTask)

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
#define CO_TASK_STATS_STACK

#else // CO_TASK_STATS

#define STAT_VAL(name) (0)
#define STAT_VAL_SET(name, value) (0)

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

CoTask *cotask_new(CoTaskFunc func) {
	CoTask *task;
	STAT_VAL_ADD(num_tasks_in_use, 1);

	if((task = alist_pop(&task_pool))) {
		koishi_recycle(&task->ko, func);
		TASK_DEBUG(
			"Recycled task %p for proc %p (%zu tasks allocated / %zu in use)",
			(void*)task, *(void**)&func,
			STAT_VAL(num_tasks_allocated), STAT_VAL(num_tasks_in_use)
		);
	} else {
		task = calloc(1, sizeof(*task));
		koishi_init(&task->ko, CO_STACK_SIZE, func);
		STAT_VAL_ADD(num_tasks_allocated, 1);
		TASK_DEBUG(
			"Created new task %p for proc %p (%zu tasks allocated / %zu in use)",
			(void*)task, *(void**)&func,
			STAT_VAL(num_tasks_allocated), STAT_VAL(num_tasks_in_use)
		);
	}

	static uint32_t unique_counter = 0;
	task->unique_id = ++unique_counter;
	setup_stack(task);
	assert(unique_counter != 0);

#ifdef CO_TASK_DEBUG
	snprintf(task->debug_label, sizeof(task->debug_label), "<unknown at %p; proc=%p>", (void*)task, *(void**)&func);
#endif

	return task;
}

void cotask_free(CoTask *task) {
	estimate_stack_usage(task);

	task->unique_id = 0;
	task->supertask = NULL;
	memset(&task->bound_ent, 0, sizeof(task->bound_ent));
	memset(&task->finalizer, 0, sizeof(task->finalizer));
	memset(&task->subtasks, 0, sizeof(task->subtasks));
	memset(&task->wait, 0, sizeof(task->wait));
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
			"Slave task %s detaching from master %s",
			task->debug_label, task->supertask->debug_label
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
			"Canceled slave task %s (of master task %s)",
			sub->debug_label, task->debug_label
		);
	}
}

static void cotask_force_cancel(CoTask *task) {
	// WARNING: It's important to finalize first, because if task == active task,
	// then koishi_kill will not return!
	cotask_finalize(task);
	TASK_DEBUG_EVENT(ev);
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
	assert(task->wait.wait_type == COTASK_WAIT_NONE);
	assert(!task->bound_ent.ent || ENT_UNBOX(task->bound_ent));

	TASK_DEBUG_EVENT(ev);
	TASK_DEBUG("[%zu] Resuming task %s", ev, task->debug_label);
	STAT_VAL_ADD(num_switches_this_frame, 1);
	arg = koishi_resume(&task->ko, arg);
	TASK_DEBUG("[%zu] koishi_resume returned (%s)", ev, task->debug_label);

	if(cotask_status(task) == CO_STATUS_DEAD) {
		cotask_finalize(task);
	}

	return arg;
}

static void *cotask_wake_and_resume(CoTask *task, void *arg) {
	task->wait.wait_type = COTASK_WAIT_NONE;
	return cotask_force_resume(task, arg);
}

CoEventSnapshot coevent_snapshot(const CoEvent *evt) {
	return (CoEventSnapshot) {
		.unique_id = evt->unique_id,
		.num_signaled = evt->num_signaled,
	};
}

CoEventStatus coevent_poll(const CoEvent *evt, const CoEventSnapshot *snap) {
	EVT_DEBUG("[%p]", (void*)evt);
	EVT_DEBUG("evt->unique_id     == %u", evt->unique_id);
	EVT_DEBUG("snap->unique_id    == %u", snap->unique_id);
	EVT_DEBUG("evt->num_signaled  == %u", evt->num_signaled);
	EVT_DEBUG("snap->num_signaled == %u", snap->num_signaled);

	if(
		evt->unique_id != snap->unique_id ||
		evt->num_signaled < snap->num_signaled
	) {
		EVT_DEBUG("Event was canceled");
		return CO_EVENT_CANCELED;
	}

	if(evt->num_signaled > snap->num_signaled) {
		EVT_DEBUG("Event was signaled");
		return CO_EVENT_SIGNALED;
	}

	EVT_DEBUG("Event hasn't changed; waiting...");
	return CO_EVENT_PENDING;
}

static bool cotask_do_wait(CoTask *task) {
	switch(task->wait.wait_type) {
		case COTASK_WAIT_NONE: {
			return false;
		}

		case COTASK_WAIT_DELAY: {
			if(--task->wait.delay.remaining < 0) {
				return false;
			}

			break;
		}

		case COTASK_WAIT_EVENT: {
			TASK_DEBUG("COTASK_WAIT_EVENT in task %s", task->debug_label);

			CoEventStatus stat = coevent_poll(task->wait.event.pevent, &task->wait.event.snapshot);
			if(stat != CO_EVENT_PENDING) {
				task->wait.result.event_status = stat;
				TASK_DEBUG("COTASK_WAIT_EVENT in task %s RESULT = %i", task->debug_label, stat);
				return false;
			}

			break;
		}
	}

	task->wait.result.frames++;
	return true;
}

void *cotask_resume(CoTask *task, void *arg) {
	if(task->bound_ent.ent && !ENT_UNBOX(task->bound_ent)) {
		cotask_force_cancel(task);
		return NULL;
	}

	if(!cotask_do_wait(task)) {
		return cotask_wake_and_resume(task, arg);
	}

	assert(task->wait.wait_type != COTASK_WAIT_NONE);
	return NULL;
}

void *cotask_yield(void *arg) {
	attr_unused CoTask *task = cotask_active();
	TASK_DEBUG_EVENT(ev);
	TASK_DEBUG("[%zu] Yielding from task %s", ev, task->debug_label);
	STAT_VAL_ADD(num_switches_this_frame, 1);
	arg = koishi_yield(arg);
	TASK_DEBUG("[%zu] koishi_yield returned (%s)", ev, task->debug_label);
	return arg;
}

static inline CoWaitResult cotask_wait_init(CoTask *task, char wait_type) {
	CoWaitResult wr = task->wait.result;
	memset(&task->wait, 0, sizeof(task->wait));
	task->wait.wait_type = wait_type;
	return wr;
}

int cotask_wait(int delay) {
	CoTask *task = cotask_active();
	assert(task->wait.wait_type == COTASK_WAIT_NONE);

	if(delay == 1) {
		cotask_yield(NULL);
		return 1;
	}

	cotask_wait_init(task, COTASK_WAIT_DELAY);
	task->wait.delay.remaining = delay;

	if(cotask_do_wait(task)) {
		cotask_yield(NULL);
	}

	return cotask_wait_init(task, COTASK_WAIT_NONE).frames;
}

static void coevent_add_subscriber(CoEvent *evt, CoTask *task) {
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

	evt->subscribers[evt->num_subscribers - 1] = cotask_box(task);
}

CoWaitResult cotask_wait_event(CoEvent *evt, void *arg) {
	assert(evt->unique_id > 0);

	CoTask *task = cotask_active();
	coevent_add_subscriber(evt, task);

	cotask_wait_init(task, COTASK_WAIT_EVENT);
	task->wait.event.pevent = evt;
	task->wait.event.snapshot = coevent_snapshot(evt);

	if(cotask_do_wait(task)) {
		cotask_yield(NULL);
	}

	return cotask_wait_init(task, COTASK_WAIT_NONE);
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
		"Made task %s a slave of task %s",
		slave->debug_label, slave->supertask->debug_label
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
			EVT_DEBUG("Resume CoEvent{%p} subscriber %s", (void*)evt, task->debug_label);
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

CoTask *_cosched_new_task(CoSched *sched, CoTaskFunc func, void *arg, CoTaskDebugInfo debug) {
	CoTask *task = cotask_new(func);
#ifdef CO_TASK_DEBUG
	snprintf(task->debug_label, sizeof(task->debug_label), "#%i <%p> %s (%s:%i:%s)", task->unique_id, (void*)task, debug.label, debug.debug_info.file, debug.debug_info.line, debug.debug_info.func);
#endif
	cotask_force_resume(task, arg);
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

#ifdef DEBUG
#include "video.h"
#include "resource/font.h"
#endif

void coroutines_draw_stats(void) {
#ifdef CO_TASK_STATS
	static char buf[128];

	TextParams tp = {
		.pos = { SCREEN_W },
		.color = RGB(1, 1, 1),
		.shader_ptr = r_shader_get("text_default"),
		.font_ptr = get_font("monotiny"),
		.align = ALIGN_RIGHT,
	};

	float ls = font_get_lineskip(tp.font_ptr);

	tp.pos.y += ls;
	snprintf(buf, sizeof(buf), "Peak stack: %zukb    Tasks: %4zu / %4zu ",
		STAT_VAL(peak_stack_usage) / 1024,
		STAT_VAL(num_tasks_in_use),
		STAT_VAL(num_tasks_allocated)
	);
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
