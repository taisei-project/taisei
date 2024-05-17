/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taskmanager.h"

#include "list.h"
#include "log.h"
#include "util/env.h"

#include <SDL_atomic.h>
#include <SDL_mutex.h>
#include <SDL_thread.h>

typedef enum TaskManagerState {
	TMGR_STATE_SHUTDOWN,
	TMGR_STATE_RUNNING,
	TMGR_STATE_ABORTED,
} TaskManagerState;

struct TaskManager {
	LIST_ANCHOR(Task) queue;
	SDL_sem *queue_sem;
	SDL_SpinLock queue_lock;
	uint numthreads;
	TaskManagerState state;
	SDL_atomic_t numtasks;
	Thread *threads[];
};

struct Task {
	LIST_INTERFACE(Task);
	task_func_t callback;
	task_free_func_t userdata_free_callback;
	void *userdata;
	int prio;
	SDL_mutex *mutex;
	SDL_cond *cond;
	TaskStatus status;
	void *result;
	uint disowned : 1;
	uint in_queue : 1;
};

static TaskManager *g_taskmgr;

static void taskmgr_free(TaskManager *mgr) {
	SDL_DestroySemaphore(mgr->queue_sem);
	mem_free(mgr);
}

static void task_free(Task *task) {
	assert(!task->in_queue);
	assert(task->disowned);

	if(task->userdata_free_callback != NULL) {
		task->userdata_free_callback(task->userdata);
	}

	if(task->mutex != NULL) {
		SDL_DestroyMutex(task->mutex);
	}

	if(task->cond != NULL) {
		SDL_DestroyCond(task->cond);
	}

	mem_free(task);
}

static Task *taskmgr_pop_queue(TaskManager *mgr) {
	if(mgr->queue.first == NULL) {
		return NULL;
	}

	SDL_AtomicLock(&mgr->queue_lock);
	auto t = alist_pop(&mgr->queue);
	SDL_AtomicUnlock(&mgr->queue_lock);
	return t;
}

static void *taskmgr_thread(void *arg) {
	TaskManager *mgr = arg;
	attr_unused SDL_threadID tid = SDL_ThreadID();

	TaskManagerState state = mgr->state;
	SDL_sem *qsem = mgr->queue_sem;

	while(state != TMGR_STATE_ABORTED) {
		SDL_SemWait(qsem);

		Task *task = taskmgr_pop_queue(mgr);
		state = mgr->state;

		if(UNLIKELY(task == NULL)) {
			if(state == TMGR_STATE_SHUTDOWN) {
				break;
			}

			continue;
		}

		SDL_LockMutex(task->mutex);

		if(state == TMGR_STATE_ABORTED && task->status == TASK_PENDING) {
			task->status = TASK_CANCELLED;
		}

		if(task->status == TASK_PENDING) {
			task->status = TASK_RUNNING;

			SDL_UnlockMutex(task->mutex);
			task->result = task->callback(task->userdata);
			SDL_LockMutex(task->mutex);

			assert(task->in_queue);
			task->in_queue = false;
			(void)SDL_AtomicDecRef(&mgr->numtasks);

			if(task->disowned) {
				SDL_UnlockMutex(task->mutex);
				task_free(task);
			} else {
				task->status = TASK_FINISHED;
				SDL_CondBroadcast(task->cond);
				SDL_UnlockMutex(task->mutex);
			}
		} else if(
			task->status == TASK_CANCELLED ||
			task->status == TASK_RUNNING ||
			task->status == TASK_FINISHED
		) {
			assert(task->in_queue);
			task->in_queue = false;
			(void)SDL_AtomicDecRef(&mgr->numtasks);
			bool task_disowned = task->disowned;
			SDL_UnlockMutex(task->mutex);

			if(task_disowned) {
				task_free(task);
			}
		} else {
			UNREACHABLE;
		}
	}

	return NULL;
}

TaskManager *taskmgr_create(uint numthreads, ThreadPriority prio, const char *name) {
	int numcores = SDL_GetCPUCount();

	if(numcores < 1) {
		log_warn("SDL_GetCPUCount() returned %i, assuming 1", numcores);
		numcores = 1;
	}

	uint maxthreads = numcores * 4;

	if(numthreads == 0) {
		numthreads = numcores * 2;
	} else if(numthreads > maxthreads) {
		log_warn("Number of threads capped to %i (%i requested)", maxthreads, numthreads);
		numthreads = maxthreads;
	}

	auto mgr = ALLOC_FLEX(TaskManager, numthreads * sizeof(SDL_Thread*));

	if(!(mgr->queue_sem = SDL_CreateSemaphore(0))) {
		log_sdl_error(LOG_ERROR, "SDL_CreateSemaphore");
		goto fail;
	}

	mgr->numthreads = numthreads;
	mgr->state = TMGR_STATE_RUNNING;

	for(uint i = 0; i < numthreads; ++i) {
		int digits = i ? log10(i) + 1 : 1;
		static const char *const prefix = "taskmgr";
		char threadname[sizeof(prefix) + strlen(name) + digits + 2];
		snprintf(threadname, sizeof(threadname), "%s:%s/%i", prefix, name, i);

		if(!(mgr->threads[i] = thread_create(threadname, taskmgr_thread, mgr, prio))) {
			mgr->state = TMGR_STATE_ABORTED;

			for(uint j = 0; j < i; ++j) {
				SDL_SemPost(mgr->queue_sem);
				thread_decref(mgr->threads[j]);
				mgr->threads[j] = NULL;
			}

			goto fail;
		}
	}

	log_debug(
		"Created task manager %s (%p) with %u threads at priority %i",
		name,
		(void*)mgr,
		mgr->numthreads,
		prio
	);

	return mgr;

fail:
	taskmgr_free(mgr);
	return NULL;
}

static int task_prio_func(List *ltask) {
	return ((Task*)ltask)->prio;
}

Task *taskmgr_submit(TaskManager *mgr, TaskParams params) {
	assert(params.callback != NULL);
	assert(mgr->state == TMGR_STATE_RUNNING);

	auto task = ALLOC(Task, {
		.callback = params.callback,
		.userdata_free_callback = params.userdata_free_callback,
		.userdata = params.userdata,
		.prio = params.prio,
		.status = TASK_PENDING,
	});

	if(!(task->mutex = SDL_CreateMutex())) {
		log_sdl_error(LOG_WARN, "SDL_CreateMutex");
		goto fail;
	}

	if(!(task->cond = SDL_CreateCond())) {
		log_sdl_error(LOG_WARN, "SDL_CreateCond");
		goto fail;
	}

	SDL_AtomicLock(&mgr->queue_lock);
	if(params.topmost) {
		alist_insert_at_priority_head(&mgr->queue, task, task->prio, task_prio_func);
	} else {
		alist_insert_at_priority_tail(&mgr->queue, task, task->prio, task_prio_func);
	}
	SDL_AtomicUnlock(&mgr->queue_lock);

	task->in_queue = true;
	SDL_AtomicIncRef(&mgr->numtasks);
	SDL_SemPost(mgr->queue_sem);

	return task;

fail:
	task_free(task);
	return NULL;
}

uint taskmgr_remaining(TaskManager *mgr) {
	return SDL_AtomicGet(&mgr->numtasks);
}

static void taskmgr_finalize_and_wait(TaskManager *mgr, bool do_abort) {
	log_debug(
		"%08lx [%p] waiting for %u tasks (abort = %i)",
		SDL_ThreadID(),
		(void*)mgr,
		taskmgr_remaining(mgr),
		do_abort
	);

	assert(mgr->state == TMGR_STATE_RUNNING);
	mgr->state = do_abort ? TMGR_STATE_ABORTED : TMGR_STATE_SHUTDOWN;

	for(uint i = 0; i < mgr->numthreads; ++i) {
		SDL_SemPost(mgr->queue_sem);
	}

	for(uint i = 0; i < mgr->numthreads; ++i) {
		thread_wait(mgr->threads[i]);
	}

	taskmgr_free(mgr);
}

void taskmgr_finish(TaskManager *mgr) {
	taskmgr_finalize_and_wait(mgr, false);
}

void taskmgr_abort(TaskManager *mgr) {
	taskmgr_finalize_and_wait(mgr, true);
}

TaskStatus task_status(Task *task) {
	TaskStatus result = TASK_INVALID;

	if(task != NULL) {
		SDL_LockMutex(task->mutex);
		result = task->status;
		SDL_UnlockMutex(task->mutex);
	}

	return result;
}

static void *task_offload(Task *task) {
	assert(task->status == TASK_PENDING);
	task->status = TASK_RUNNING;
	SDL_UnlockMutex(task->mutex);
	void *result = task->callback(task->userdata);
	SDL_LockMutex(task->mutex);
	assert(!task->disowned);
	task->status = TASK_FINISHED;
	task->result = result;
	SDL_CondBroadcast(task->cond);
	return result;
}

bool task_wait(Task *task, void **result) {
	bool success = false;

	if(task == NULL) {
		return success;
	}

	void *_result = NULL;

	SDL_LockMutex(task->mutex);

	if(task->status == TASK_CANCELLED) {
		success = false;
	} else if(task->status == TASK_FINISHED) {
		success = true;
		_result = task->result;
	} else if(task->status == TASK_RUNNING) {
		SDL_CondWait(task->cond, task->mutex);
		_result = task->result;
		success = (task->status == TASK_FINISHED);
	} else if(task->status == TASK_PENDING) {
		// fine, i'll do it myself
		_result = task_offload(task);
		success = true;
	} else {
		UNREACHABLE;
	}

	SDL_UnlockMutex(task->mutex);

	if(success && result != NULL) {
		*result = _result;
	}

	return success;
}

bool task_cancel(Task *task) {
	bool success = false;

	if(task == NULL) {
		return success;
	}

	SDL_LockMutex(task->mutex);

	if(task->status == TASK_PENDING) {
		task->status = TASK_CANCELLED;
		success = true;
	}

	SDL_UnlockMutex(task->mutex);

	return success;
}

bool task_detach(Task *task) {
	bool success = false;
	bool task_in_queue;

	if(task == NULL) {
		return success;
	}

	SDL_LockMutex(task->mutex);
	assert(!task->disowned);
	task->disowned = true;
	task_in_queue = task->in_queue;
	success = true;
	SDL_UnlockMutex(task->mutex);

	if(!task_in_queue) {
		task_free(task);
	}

	return success;
}

bool task_finish(Task *task, void **result) {
	bool success = task_wait(task, result);
	task_detach(task);
	return success;
}

bool task_abort(Task *task) {
	bool success = task_cancel(task);
	task_detach(task);
	return success;
}

void taskmgr_global_init(void) {
	assert(g_taskmgr == NULL);
	int nthreads = env_get("TAISEI_TASKMGR_NUM_THREADS", 0);
	g_taskmgr = taskmgr_create(nthreads, SDL_THREAD_PRIORITY_LOW, "global");
}

void taskmgr_global_shutdown(void) {
	if(g_taskmgr != NULL) {
		taskmgr_finish(g_taskmgr);
		g_taskmgr = NULL;
	}
}

Task *taskmgr_global_submit(TaskParams params) {
	if(g_taskmgr == NULL) {
		return ALLOC(Task, {
			.callback = params.callback,
			.userdata = params.userdata,
			.userdata_free_callback = params.userdata_free_callback,
			.result = params.callback(params.userdata),
		});
	}

	return taskmgr_submit(g_taskmgr, params);
}
