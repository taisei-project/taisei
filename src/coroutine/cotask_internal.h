/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#ifdef __EMSCRIPTEN__
	#define CO_STACK_SIZE (64 * 1024)
#else
	#define CO_STACK_SIZE (256 * 1024)
#endif

#ifdef CO_TASK_DEBUG
	#define TASK_DEBUG(...) log_debug(__VA_ARGS__)
	extern size_t _cotask_debug_event_id;
	#define TASK_DEBUG_EVENT(ev) uint64_t ev = _cotask_debug_event_id++
#else
	#define TASK_DEBUG(...) ((void)0)
	#define TASK_DEBUG_EVENT(ev) ((void)0)
#endif

#ifdef DEBUG
	#define CO_TASK_STATS
#endif

#ifdef ADDRESS_SANITIZER
	#include <sanitizer/asan_interface.h>
#else
	#define ASAN_UNPOISON_MEMORY_REGION(addr, size) ((void)0)
#endif

#define MEM_AREA_SIZE (1 << 12)
#define MEM_ALLOC_ALIGNMENT alignof(max_align_t)
#define MEM_ALIGN_SIZE(x) (x + (MEM_ALLOC_ALIGNMENT - 1)) & ~(MEM_ALLOC_ALIGNMENT - 1)

enum {
	COTASK_WAIT_NONE,
	COTASK_WAIT_DELAY,
	COTASK_WAIT_EVENT,
	COTASK_WAIT_SUBTASKS,
};

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

#ifdef CO_TASK_STATS
typedef struct CoTaskStats {
	size_t num_tasks_allocated;
	size_t num_tasks_in_use;
	size_t num_switches_this_frame;
	size_t peak_stack_usage;
} CoTaskStats;
extern CoTaskStats cotask_stats;

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

typedef struct CoTaskHeapMemChunk {
	struct CoTaskHeapMemChunk *next;
	alignas(MEM_ALLOC_ALIGNMENT) char data[];
} CoTaskHeapMemChunk;

struct CoTaskData {
	LIST_INTERFACE(CoTaskData);

	CoTask *task;
	CoSched *sched;

	CoTaskData *master;              // AKA supertask
	LIST_ANCHOR(CoTaskData) slaves;  // AKA subtasks

	BoxedEntity bound_ent;
	CoTaskEvents events;

	bool finalizing;

	struct {
		CoWaitResult result;

		union {
			struct {
				int remaining;
			} delay;

			struct {
				CoEvent *pevent;
				CoEventSnapshot snapshot;
			} event;
		};

		uint wait_type;
	} wait;

	struct {
		EntityInterface *ent;
		CoEvent *events;
		uint num_events;
	} hosted;

	struct {
		CoTaskHeapMemChunk *onheap_alloc_head;
		char *onstack_alloc_head;
		alignas(MEM_ALLOC_ALIGNMENT) char onstack_alloc_area[MEM_AREA_SIZE];
	} mem;
};

typedef struct CoTaskInitData {
	CoTask *task;
	CoSched *sched;
	CoTaskFunc func;
	void *func_arg;
	size_t func_arg_size;
	CoTaskData *master_task_data;
} CoTaskInitData;

void cotask_global_init(void);
void cotask_global_shutdown(void);

CoTask *cotask_new_internal(koishi_entrypoint_t entry_point);
void *cotask_resume_internal(CoTask *task, void *arg);
CoTask *cotask_unbox_notnull(BoxedTask box);
void cotask_force_finish(CoTask *task);
void *cotask_entry(void *varg);

attr_returns_nonnull attr_nonnull_all
INLINE CoTaskData *cotask_get_data(CoTask *task) {
	CoTaskData *data = task->data;
	assume(data != NULL);
	assume(data->task == task);
	return data;
}
