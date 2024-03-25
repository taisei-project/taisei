/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include <koishi.h>

#include "list.h"
#include "entity.h"
#include "util/debug.h"

// #define CO_TASK_DEBUG

typedef struct CoTask CoTask;
typedef LIST_ANCHOR(CoTask) CoTaskList;
typedef void *(*CoTaskFunc)(void *arg, size_t argsize);

typedef enum CoStatus {
	CO_STATUS_SUSPENDED = KOISHI_SUSPENDED,
	CO_STATUS_RUNNING   = KOISHI_RUNNING,
	CO_STATUS_IDLE      = KOISHI_IDLE,
	CO_STATUS_DEAD      = KOISHI_DEAD,
} CoStatus;

typedef struct BoxedTask {
	alignas(alignof(void*)) uintptr_t ptr;
	uint32_t unique_id;
} BoxedTask;

#include "coevent.h"

typedef struct CoWaitResult {
	int frames;
	CoEventStatus event_status;
} CoWaitResult;

typedef struct CoTaskDebugInfo {
	const char *label;
#ifdef CO_TASK_DEBUG
	DebugInfo debug_info;
#endif
} CoTaskDebugInfo;

#ifdef CO_TASK_DEBUG
	#define COTASK_DEBUG_INFO(label) ((CoTaskDebugInfo) { (label), _DEBUG_INFO_INITIALIZER_ })
#else
	#define COTASK_DEBUG_INFO(label) ((CoTaskDebugInfo) { (label) })
#endif

typedef struct CoSched CoSched;

void cotask_free(CoTask *task);
bool cotask_cancel(CoTask *task);
void *cotask_resume(CoTask *task, void *arg);
void *cotask_yield(void *arg);
int cotask_wait(int delay);
CoWaitResult cotask_wait_event(CoEvent *evt);
CoWaitResult cotask_wait_event_or_die(CoEvent *evt);
CoWaitResult cotask_wait_event_once(CoEvent *evt);
int cotask_wait_subtasks(void);
CoStatus cotask_status(CoTask *task);
CoTask *cotask_active(void);  // Returns NULL if not in task context
CoTask *cotask_active_unsafe(void) attr_returns_nonnull;  // Must be called in task context
EntityInterface *cotask_bind_to_entity(CoTask *task, EntityInterface *ent) attr_returns_nonnull;
CoTaskEvents *cotask_get_events(CoTask *task);
void *cotask_malloc(CoTask *task, size_t size) attr_returns_allocated attr_malloc attr_alloc_size(2);
EntityInterface *cotask_host_entity(CoTask *task, size_t ent_size, EntityType ent_type) attr_nonnull_all attr_returns_allocated;
void cotask_host_events(CoTask *task, uint num_events, CoEvent events[num_events]) attr_nonnull_all;
CoSched *cotask_get_sched(CoTask *task);
const char *cotask_get_name(CoTask *task) attr_nonnull(1);

BoxedTask cotask_box(CoTask *task);
CoTask *cotask_unbox(BoxedTask box);
