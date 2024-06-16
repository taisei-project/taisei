/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "cotask.h"

typedef struct CoSched CoSched;

struct CoSched {
	CoTaskList tasks, pending_tasks;
};

void cosched_init(CoSched *sched);
CoTask *_cosched_new_task(CoSched *sched, CoTaskFunc func, void *arg, size_t arg_size, bool is_subtask, CoTaskDebugInfo debug);  // creates and runs the task, schedules it for resume on cosched_run_tasks if it's still alive
#define cosched_new_task(sched, func, arg, arg_size, debug_label) \
	_cosched_new_task(sched, func, arg, arg_size, false, COTASK_DEBUG_INFO(debug_label))
#define cosched_new_subtask(sched, func, arg, arg_size, debug_label) \
	_cosched_new_task(sched, func, arg, arg_size, true, COTASK_DEBUG_INFO(debug_label))
uint cosched_run_tasks(CoSched *sched);  // returns number of tasks ran
void cosched_finish(CoSched *sched);
