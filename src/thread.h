/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

void thread_init(void);
void thread_shutdown(void);

typedef struct Thread Thread;
typedef uint64_t ThreadID;

typedef void *(*ThreadProc)(void *userdata);

typedef enum ThreadPriority {
	THREAD_PRIO_LOW,
	THREAD_PRIO_NORMAL,
	THREAD_PRIO_HIGH,
	THREAD_PRIO_CRITICAL,
} ThreadPriority;

/*
 * Creates and starts executing a "managed" thread.
 * Other threads not started by this function are known as "foreign", including the main thread.
 *
 * Thread objects have a reference count, which starts at 1.
 * You can manipulate it via thread_incref() and thread_decref(); thread_wait() also decrements the
 * reference count.
 *
 * Returns NULL if threads are not supported.
 */
Thread *thread_create(const char *name, ThreadProc proc, void *userdata, ThreadPriority prio)
	attr_returns_max_aligned
	attr_nonnull(1, 2);

/*
 * Increments the thread's reference count.
 */
void thread_incref(Thread *thrd)
	attr_nonnull_all;

/*
 * Decrements the thread's reference count, releasing resources if it hits zero.
 *
 * If the reference count hits zero while the thread is running, the cleanup is delayed until after
 * it's done executing, iff the reference count stays at 0 by then. Thus this is somewhat
 * equivalent to pthread_detach, except it can be canceled by incrementing the refcount while the
 * thread is still running. That is only guaranteed to be safe when doing so from within that
 * thread, however.
 */
void thread_decref(Thread *thrd)
	attr_nonnull_all;

/*
 * Returns the executing managed Thread.
 * For foreign threads this function returns NULL.
 */
Thread *thread_get_current(void);

/*
 * Returns a unique ID of the Thread.
 * The meaning of this value is system-specific.
 */
ThreadID thread_get_id(Thread *thrd)
	attr_nonnull_all;

/*
 * Returns a unique ID of the currently executing thread.
 * This works even if the thread is foreign.
 * For managed threads, this function returns the same value as thread_get_id(thread_get_current())
 */
ThreadID thread_get_current_id(void)
	attr_pure;

/*
 * Returns the unique id of the main thread.
 */
ThreadID thread_get_main_id(void)
	attr_pure;

/*
 * Returns true if the executing thread is the main thread.
 */
bool thread_current_is_main(void)
	attr_pure;

/*
 * Returns the name assigned to a managed thread at creation.
 */
const char *thread_get_name(Thread *thrd)
	attr_returns_nonnull
	attr_nonnull_all;

/*
 * Waits for a thread to finish executing.
 * Returns the entry point's return value.
 * The thread can't be referenced again after this function returns.
 *
 * You must eventually wait on all threads you create, or detach them via thread_decref().
 * Not doing so may result in a "zombie" thread preventing the main program from exiting.
 *
 * WARNING: This function also decrements the thread's reference count after the wait is done.
 * You should not wait on a "detached" thread.
 */
void *thread_wait(Thread *thrd)
	attr_nonnull_all;

/*
 * Returns true if the thread has finished executing, and stores its return value into `result`.
 */
bool thread_get_result(Thread *thrd, void **result)
	attr_nodiscard
	attr_nonnull(1);
