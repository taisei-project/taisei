/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "thread.h"

typedef struct TaskManager TaskManager;
typedef struct Task Task;

typedef enum TaskStatus {
	TASK_INVALID,    /** Indicates an error */
	TASK_PENDING,    /** Task is still in queue and can be cancelled */
	TASK_RUNNING,    /** Task is currently executing and can not be cancelled */
	TASK_CANCELLED,  /** Task has been cancelled and will not run */
	TASK_FINISHED,   /** Task has finished executing and has been removed from the queue */
} TaskStatus;

typedef void *(*task_func_t)(void *userdata);
typedef void (*task_free_func_t)(void *userdata);

/**
 * Parameters for `taskmgr_submit`. See its documentation below.
 */
typedef struct TaskParams {
	/**
	 * The function implementing the task. Must not be NULL.
	 * The return value may be anything. It can be obtained with `task_wait` or `task_finish`.
	 */
	task_func_t callback;

	/**
	 * Value passed as argument to callback.
	 */
	void *userdata;

	/**
	 * Unless NULL, this function will be called when userdata should be freed.
	 * You should use it instead of releasing resources in the task function. It's guaranteed to
	 * be called even if the task has been cancelled.
	 *
	 * This function may be called from any thread, so be careful what you do with it.
	 */
	task_free_func_t userdata_free_callback;

	/**
	 * Priority of the task. Lower values mean higher priority. Higher priority tasks are added
	 * to the queue ahead of the lower priority ones, and thus will start executing sooner. Note
	 * that this affects only the pending tasks. A task that already began executing cannot be
	 * interrupted, regardless of its priority.
	 */
	int prio;

	/**
	 * If true, this task will be inserted ahead of the others with the same priority, if any.
	 * Otherwise, it'll be put behind them instead.
	 */
	bool topmost;
} TaskParams;

/**
 * Create a new TaskManager with [numthreads] worker threads, and set their priority to [prio].
 * If [numthreads] is 0, a default based on the amount of system's CPU cores will be used.
 * The actual amount of threads spawned may be lower than requested.
 *
 * [name] is used to form names of the worker threads. Keep it short.
 *
 * On success, returns a pointer to the created TaskManager.
 * On failure, returns NULL.
 */
TaskManager *taskmgr_create(uint numthreads, ThreadPriority prio, const char *name)
	attr_nodiscard attr_returns_max_aligned attr_nonnull(3);

/**
 * Submit a new task to [mgr] described by [params]. It is generally placed at the end of the
 * task manager's queue, but that can be influenced with [params.prio] and [params.topmost].
 *
 * See documentation for TaskParams above.
 *
 * However, you should not rely on the tasks being actually executed in any specific order, in
 * particular if the task manager is multi-threaded.
 *
 * On success, returns a pointer to a Task structure, which must be eventually passed to one of
 * `task_detach`, `task_finish`, or `task_abort`. Not doing so is a resource leak.
 *
 * On failure, returns NULL.
 */
Task *taskmgr_submit(TaskManager *mgr, TaskParams params)
	attr_nonnull(1) attr_nodiscard attr_returns_max_aligned;

/**
 * Returns the number of remaining tasks in [mgr]'s queue.
 */
uint taskmgr_remaining(TaskManager *mgr)
	attr_nonnull(1);

/**
 * Wait for all remaining tasks to complete, then destroy [mgr], freeing associated resources.
 * [mgr] must be treated as an invalid pointer as soon as this function is called.
 */
void taskmgr_finish(TaskManager *mgr)
	attr_nonnull(1);

/**
 * Cancel all pending tasks, wait for all running tasks to complete, then destroy [mgr], freeing
 * associated resources. [mgr] must be treated as an invalid pointer as soon as this function
 * is called.
 */
void taskmgr_abort(TaskManager *mgr)
	attr_nonnull(1);

/**
 * Returns the current status of [task]. See TaskStatus documentation above.
 * Returns TASK_INVALID on failure.
 */
TaskStatus task_status(Task *task);

/**
 * Wait for [task] to complete. If the task is not running yet, it will be removed from the queue and
 * executed on the current thread instead.
 *
 * On success, returns true and stores the task's return value in [result] (unless [result] is NULL).
 * On failure, returns false; [result] is left untouched.
 */
bool task_wait(Task *task, void **result);

/**
 * Cancel a pending [task].
 * This does not stop an already running task.
 *
 * Returns true on success, false on failure.
 */
bool task_cancel(Task *task);

/**
 * Disown the [task]. Resources associated with this task will be eventually freed by its TaskManager,
 * or immediately after this call if the task is no longer in the queue. Use this when you are done
 * with the task.
 *
 * [task] must be treated as an invalid pointer as soon as this function is called.
 *
 * Returns true on success, false on failure.
 */
bool task_detach(Task *task);

/**
 * Wait for [task] to complete, then disown it.
 * Functionally equivalent to `task_wait` followed by `task_detach`.
 */
bool task_finish(Task *task, void **result);

/**
 * Attempt to cancel the [task], then disown it.
 * Functionally equivalent to `task_cancel` followed by `task_detach`.
 */
bool task_abort(Task *task);

/**
 * Initialize the global task manager with default parameters.
 */
void taskmgr_global_init(void);

/**
 * Wait for tasks submitted to the global task manager to finish, then destroy
 * it, freeing all associated resources.
 */
void taskmgr_global_shutdown(void);

/**
 * Submit a task to the global task manager. See `taskmgr_submit`.
 */
Task *taskmgr_global_submit(TaskParams params);
