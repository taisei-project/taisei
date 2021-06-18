/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "taskmanager.h"
#include "list.h"
#include "util.h"

struct TaskManager {
	LIST_ANCHOR(Task) queue;
	SDL_mutex *mutex;
	SDL_cond *cond;
	uint numthreads;
	uint running : 1;
	uint aborted : 1;
	SDL_atomic_t numtasks;
	SDL_ThreadPriority thread_prio;
	SDL_Thread *threads[];
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
	if(mgr->mutex != NULL) {
		SDL_DestroyMutex(mgr->mutex);
	}

	if(mgr->cond != NULL) {
		SDL_DestroyCond(mgr->cond);
	}

	free(mgr);
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

	free(task);
}

static int taskmgr_thread(void *arg) {
	TaskManager *mgr = arg;
	attr_unused SDL_threadID tid = SDL_ThreadID();

	if(SDL_SetThreadPriority(mgr->thread_prio) < 0) {
		log_sdl_error(LOG_WARN, "SDL_SetThreadPriority");
	}

	bool running;
	bool aborted;

	do {
		SDL_LockMutex(mgr->mutex);

		running = mgr->running;
		aborted = mgr->aborted;

		if(!running && !aborted) {
			SDL_CondWait(mgr->cond, mgr->mutex);
		}

		SDL_UnlockMutex(mgr->mutex);
	} while(!running && !aborted);

	while(!aborted) {
		SDL_LockMutex(mgr->mutex);
		Task *task = alist_pop(&mgr->queue);

		running = mgr->running;
		aborted = mgr->aborted;

		if(running && task == NULL && !aborted) {
			SDL_CondWait(mgr->cond, mgr->mutex);
		}

		SDL_UnlockMutex(mgr->mutex);

		if(task != NULL) {
			SDL_LockMutex(task->mutex);

			if(aborted && task->status == TASK_PENDING) {
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
			} else if(task->status == TASK_CANCELLED || task->status == TASK_RUNNING || task->status == TASK_FINISHED) {
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
		} else if(!running) {
			break;
		}
	}

	return 0;
}

TaskManager *taskmgr_create(uint numthreads, SDL_ThreadPriority prio, const char *name) {
	int numcores = SDL_GetCPUCount();

	if(numcores < 1) {
		log_warn("SDL_GetCPUCount() returned %i, assuming 1", numcores);
		numcores = 1;
	}

	uint maxthreads = numcores * 4;

	if(numthreads == 0) {
		numthreads = numcores;
	} else if(numthreads > maxthreads) {
		log_warn("Number of threads capped to %i (%i requested)", maxthreads, numthreads);
		numthreads = maxthreads;
	}

	TaskManager *mgr = calloc(1, sizeof(TaskManager) + numthreads * sizeof(SDL_Thread*));

	if(!(mgr->mutex = SDL_CreateMutex())) {
		log_sdl_error(LOG_WARN, "SDL_CreateMutex");
		goto fail;
	}

	if(!(mgr->cond = SDL_CreateCond())) {
		log_sdl_error(LOG_WARN, "SDL_CreateCond");
		goto fail;
	}

	mgr->numthreads = numthreads;
	mgr->thread_prio = prio;

	for(uint i = 0; i < numthreads; ++i) {
		int digits = i ? log10(i) + 1 : 0;
		static const char *const prefix = "taskmgr";
		char threadname[sizeof(prefix) + strlen(name) + digits + 2];
		snprintf(threadname, sizeof(threadname), "%s:%s/%i", prefix, name, i);

		if(!(mgr->threads[i] = SDL_CreateThread(taskmgr_thread, threadname, mgr))) {
			log_sdl_error(LOG_WARN, "SDL_CreateThread");

			for(uint j = 0; j < i; ++j) {
				SDL_DetachThread(mgr->threads[j]);
				mgr->threads[j] = NULL;
			}

			SDL_LockMutex(mgr->mutex);
			mgr->aborted = true;
			SDL_CondBroadcast(mgr->cond);
			SDL_UnlockMutex(mgr->mutex);
			goto fail;
		}
	}

	SDL_LockMutex(mgr->mutex);
	mgr->running = true;
	SDL_CondBroadcast(mgr->cond);
	SDL_UnlockMutex(mgr->mutex);

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

	Task *task = calloc(1, sizeof(Task));
	task->callback = params.callback;
	task->userdata_free_callback = params.userdata_free_callback;
	task->userdata = params.userdata;
	task->prio = params.prio;
	task->status = TASK_PENDING;

	if(!(task->mutex = SDL_CreateMutex())) {
		log_sdl_error(LOG_WARN, "SDL_CreateMutex");
		goto fail;
	}

	if(!(task->cond = SDL_CreateCond())) {
		log_sdl_error(LOG_WARN, "SDL_CreateCond");
		goto fail;
	}

	SDL_LockMutex(mgr->mutex);

	if(params.topmost) {
		alist_insert_at_priority_head(&mgr->queue, task, task->prio, task_prio_func);
	} else {
		alist_insert_at_priority_tail(&mgr->queue, task, task->prio, task_prio_func);
	}

	task->in_queue = true;
	SDL_AtomicIncRef(&mgr->numtasks);
	SDL_CondSignal(mgr->cond);
	SDL_UnlockMutex(mgr->mutex);

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

	assert(mgr->running);
	assert(!mgr->aborted);

	SDL_LockMutex(mgr->mutex);
	mgr->running = false;
	mgr->aborted = do_abort;
	SDL_CondBroadcast(mgr->cond);
	SDL_UnlockMutex(mgr->mutex);

	for(uint i = 0; i < mgr->numthreads; ++i) {
		SDL_WaitThread(mgr->threads[i], NULL);
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
	g_taskmgr = taskmgr_create(0, SDL_THREAD_PRIORITY_LOW, "global");
}

void taskmgr_global_shutdown(void) {
	if(g_taskmgr != NULL) {
		taskmgr_finish(g_taskmgr);
		g_taskmgr = NULL;
	}
}

Task *taskmgr_global_submit(TaskParams params) {
	if(g_taskmgr == NULL) {
		Task *t = calloc(1, sizeof(Task));
		t->callback = params.callback;
		t->userdata = params.userdata;
		t->userdata_free_callback = params.userdata_free_callback;
		t->result = params.callback(params.userdata);
		return t;
	}

	return taskmgr_submit(g_taskmgr, params);
}
