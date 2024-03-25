/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "thread.h"
#include "log.h"
#include "hashtable.h"

#include <SDL_thread.h>

static_assert(THREAD_PRIO_LOW == (int)SDL_THREAD_PRIORITY_LOW);
static_assert(THREAD_PRIO_NORMAL == (int)SDL_THREAD_PRIORITY_NORMAL);
static_assert(THREAD_PRIO_HIGH == (int)SDL_THREAD_PRIORITY_HIGH);
static_assert(THREAD_PRIO_CRITICAL == (int)SDL_THREAD_PRIORITY_TIME_CRITICAL);

enum ThreadState {
	THREAD_STATE_EXECUTING,
	THREAD_STATE_DONE,
	THREAD_STATE_CLEANING_UP,
};

struct Thread {
	SDL_Thread *sdlthread;
	void *data;
	ThreadID id;
	SDL_atomic_t refcount;
	SDL_atomic_t state;
	char name[];
};

static struct {
	ht_int2ptr_ts_t id_to_thread;
	ThreadID main_id;
} threads;

void thread_init(void) {
	threads.main_id = SDL_ThreadID();
	ht_create(&threads.id_to_thread);
}

void thread_shutdown(void) {
	ht_int2ptr_ts_iter_t iter;
	ht_iter_begin(&threads.id_to_thread, &iter);
	for(;iter.has_data; ht_iter_next(&iter)) {
		Thread *thrd = iter.value;
		int nref = SDL_AtomicGet(&thrd->refcount);
		log_error("Thread '%s' had non-zero refcount of %i", thrd->name, nref);

		SDL_Thread *sdlthread = SDL_AtomicSetPtr((void**)&thrd->sdlthread, NULL);

		if(sdlthread) {
			SDL_DetachThread(sdlthread);
		}
	}
	ht_iter_end(&iter);

	ht_destroy(&threads.id_to_thread);
	threads.id_to_thread.num_elements_allocated = 0;
}

static void thread_finalize(Thread *thrd) {
	assert(thrd->sdlthread == NULL);
	ht_unset(&threads.id_to_thread, thrd->id);
	mem_free(thrd);
}

static void thread_try_finalize(Thread *thrd) {
	if(SDL_AtomicCAS(&thrd->state, THREAD_STATE_DONE, THREAD_STATE_CLEANING_UP)) {
		thread_finalize(thrd);
	}
}

static void thread_try_detach(Thread *thrd) {
	SDL_Thread *sdlthread = SDL_AtomicSetPtr((void**)&thrd->sdlthread, NULL);
	if(sdlthread) {
		SDL_DetachThread(sdlthread);
	}
}

void thread_incref(Thread *thrd) {
	SDL_AtomicIncRef(&thrd->refcount);
}

static void thread_decref_internal(Thread *thrd, bool try_detach) {
	int prev_refcount = SDL_AtomicAdd(&thrd->refcount, -1);
	assert(prev_refcount > 0);
	if(prev_refcount == 1) {
		thread_try_finalize(thrd);
	}
}

void thread_decref(Thread *thrd) {
	thread_decref_internal(thrd, true);
}

typedef struct ThreadCreateData {
	Thread *thrd;
	ThreadProc proc;
	void *userdata;
	ThreadPriority prio;
	SDL_sem *init_sem;
} ThreadCreateData;

static int SDLCALL sdlthread_entry(void *data) {
	auto tcd = *(ThreadCreateData*)data;

	ThreadID id = SDL_ThreadID();
	tcd.thrd->id = id;
	ht_set(&threads.id_to_thread, id, tcd.thrd);
	SDL_SemPost(tcd.init_sem);

	if(SDL_SetThreadPriority(tcd.prio) < 0) {
		log_sdl_error(LOG_WARN, "SDL_SetThreadPriority");
	}

	tcd.thrd->data = tcd.proc(tcd.userdata);

	attr_unused bool cas_ok;
	cas_ok = SDL_AtomicCAS(&tcd.thrd->state, THREAD_STATE_EXECUTING, THREAD_STATE_DONE);
	assert(cas_ok);

	if(SDL_AtomicGet(&tcd.thrd->refcount) < 1) {
		thread_try_detach(tcd.thrd);
		thread_try_finalize(tcd.thrd);
	}

	return 0;
}

Thread *thread_create(const char *name, ThreadProc proc, void *userdata, ThreadPriority prio) {
	size_t nsize = strlen(name) + 1;
	auto thrd = ALLOC_FLEX(Thread, nsize);
	memcpy(thrd->name, name, nsize);
	thrd->refcount.value = 1;
	thrd->state.value = THREAD_STATE_EXECUTING;

	ThreadCreateData tcd = {
		.prio = prio,
		.proc = proc,
		.thrd = thrd,
		.userdata = userdata,
		.init_sem = SDL_CreateSemaphore(0),
	};

	if(UNLIKELY(!tcd.init_sem)) {
		log_sdl_error(LOG_ERROR, "SDL_CreateSemaphore");
		goto fail;
	}

	thrd->sdlthread = SDL_CreateThread(sdlthread_entry, name, &tcd);

	if(UNLIKELY(!thrd->sdlthread)) {
		log_sdl_error(LOG_ERROR, "SDL_CreateThread");
		goto fail;
	}

	SDL_SemWait(tcd.init_sem);
	SDL_DestroySemaphore(tcd.init_sem);
	return thrd;

fail:
	SDL_DestroySemaphore(tcd.init_sem);
	mem_free(thrd);
	return NULL;
}

Thread *thread_get_current(void) {
	if(!threads.id_to_thread.num_elements_allocated) {
		return NULL;
	}

	return ht_get(&threads.id_to_thread, thread_get_current_id(), NULL);
}

ThreadID thread_get_id(Thread *thrd) {
	return thrd->id;
}

ThreadID thread_get_current_id(void) {
	return SDL_ThreadID();
}

ThreadID thread_get_main_id(void) {
	return threads.main_id;
}

bool thread_current_is_main(void) {
	return thread_get_current_id() == threads.main_id;
}

const char *thread_get_name(Thread *thrd) {
	return thrd->name;
}

void *thread_wait(Thread *thrd) {
	SDL_Thread *sdlthread = SDL_AtomicSetPtr((void**)&thrd->sdlthread, NULL);

	if(sdlthread) {
		SDL_WaitThread(sdlthread, NULL);
	}

	void *r = thrd->data;
	thread_decref_internal(thrd, false);
	return r;
}

bool thread_get_result(Thread *thrd, void **result) {
	if(SDL_AtomicGet(&thrd->state) == THREAD_STATE_EXECUTING) {
		return false;
	}

	if(result) {
		*result = thrd->data;
	}

	return true;
}
