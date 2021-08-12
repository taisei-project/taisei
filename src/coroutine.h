/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_coroutine_h
#define IGUARD_coroutine_h

#include "taisei.h"

#include "entity.h"
#include "util/debug.h"
#include "dynarray.h"

#include <koishi.h>

// #define CO_TASK_DEBUG

typedef struct CoTask CoTask;
typedef struct CoSched CoSched;
typedef void *(*CoTaskFunc)(void *arg, size_t argsize);

// target for the INVOKE_TASK macro
extern CoSched *_cosched_global;

typedef enum CoStatus {
	CO_STATUS_SUSPENDED = KOISHI_SUSPENDED,
	CO_STATUS_RUNNING   = KOISHI_RUNNING,
	CO_STATUS_IDLE      = KOISHI_IDLE,
	CO_STATUS_DEAD      = KOISHI_DEAD,
} CoStatus;

typedef enum CoEventStatus {
	CO_EVENT_PENDING,
	CO_EVENT_SIGNALED,
	CO_EVENT_CANCELED,
} CoEventStatus;

typedef struct BoxedTask {
	alignas(alignof(void*)) uintptr_t ptr;
	uint32_t unique_id;
} BoxedTask;

typedef struct CoEvent {
	// ListAnchor subscribers;
	// FIXME: Is there a better way than a dynamic array?
	// An intrusive linked list just isn't robust enough.
	DYNAMIC_ARRAY(BoxedTask) subscribers;
	uint32_t unique_id;
	uint32_t num_signaled;
} CoEvent;

typedef struct CoEventSnapshot {
	uint32_t unique_id;
	uint32_t num_signaled;
} CoEventSnapshot;

#define COEVENTS_ARRAY(...) \
	union { \
		CoEvent _first_event_; \
		struct { CoEvent __VA_ARGS__; }; \
	}

typedef COEVENTS_ARRAY(
	finished
) CoTaskEvents;

typedef LIST_ANCHOR(CoTask) CoTaskList;

struct CoSched {
	CoTaskList tasks, pending_tasks;
};

typedef struct CoWaitResult {
	int frames;
	CoEventStatus event_status;
} CoWaitResult;

#ifdef CO_TASK_DEBUG
typedef struct CoTaskDebugInfo {
	const char *label;
	DebugInfo debug_info;
} CoTaskDebugInfo;

#define COTASK_DEBUG_INFO(label) ((CoTaskDebugInfo) { (label), _DEBUG_INFO_INITIALIZER_ })
#else
typedef char CoTaskDebugInfo;
#define COTASK_DEBUG_INFO(label) (0)
#endif

void coroutines_init(void);
void coroutines_shutdown(void);
void coroutines_draw_stats(void);

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
CoTask *cotask_active(void);
EntityInterface *cotask_bind_to_entity(CoTask *task, EntityInterface *ent) attr_returns_nonnull;
CoTaskEvents *cotask_get_events(CoTask *task);
void *cotask_malloc(CoTask *task, size_t size) attr_returns_allocated attr_malloc attr_alloc_size(2);
EntityInterface *cotask_host_entity(CoTask *task, size_t ent_size, EntityType ent_type) attr_nonnull_all attr_returns_allocated;
void cotask_host_events(CoTask *task, uint num_events, CoEvent events[num_events]) attr_nonnull_all;

BoxedTask cotask_box(CoTask *task);
CoTask *cotask_unbox(BoxedTask box);

void coevent_init(CoEvent *evt);
void coevent_signal(CoEvent *evt);
void coevent_signal_once(CoEvent *evt);
void coevent_cancel(CoEvent *evt);
CoEventSnapshot coevent_snapshot(const CoEvent *evt);
CoEventStatus coevent_poll(const CoEvent *evt, const CoEventSnapshot *snap);

void _coevent_array_action(uint num, CoEvent *events, void (*func)(CoEvent*));

#define COEVENT_ARRAY_ACTION(func, array) (_coevent_array_action(sizeof(array)/sizeof(CoEvent), &((array)._first_event_), func))
#define COEVENT_INIT_ARRAY(array) COEVENT_ARRAY_ACTION(coevent_init, array)
#define COEVENT_CANCEL_ARRAY(array) COEVENT_ARRAY_ACTION(coevent_cancel, array)

void cosched_init(CoSched *sched);
CoTask *_cosched_new_task(CoSched *sched, CoTaskFunc func, void *arg, size_t arg_size, bool is_subtask, CoTaskDebugInfo debug);  // creates and runs the task, schedules it for resume on cosched_run_tasks if it's still alive
#define cosched_new_task(sched, func, arg, arg_size, debug_label) \
	_cosched_new_task(sched, func, arg, arg_size, false, COTASK_DEBUG_INFO(debug_label))
#define cosched_new_subtask(sched, func, arg, arg_size, debug_label) \
	_cosched_new_task(sched, func, arg, arg_size, true, COTASK_DEBUG_INFO(debug_label))
uint cosched_run_tasks(CoSched *sched);  // returns number of tasks ran
void cosched_finish(CoSched *sched);

INLINE void cosched_set_invoke_target(CoSched *sched) { _cosched_global = sched; }

#define TASK_ARGS_TYPE(name) COARGS_##name

#define TASK_ARGSDELAY_NAME(name) COARGSDELAY_##name
#define TASK_ARGSDELAY(name) struct TASK_ARGSDELAY_NAME(name)

#define TASK_ARGSCOND_NAME(name) COARGSCOND_##name
#define TASK_ARGSCOND(name) struct TASK_ARGSCOND_NAME(name)

#define TASK_IFACE_NAME(iface, suffix) COTASKIFACE_##iface##_##suffix
#define TASK_IFACE_ARGS_TYPE(iface) TASK_IFACE_NAME(iface, ARGS)
#define TASK_IFACE_ARGS_SIZED_PTR_TYPE(iface) TASK_IFACE_NAME(iface, ARGS_SPTR)
#define TASK_INDIRECT_TYPE(iface) TASK_IFACE_NAME(iface, HANDLE)

#define TASK_IFACE_SARGS(iface, ...) \
	((TASK_IFACE_ARGS_SIZED_PTR_TYPE(iface)) { \
		.size = sizeof(TASK_IFACE_ARGS_TYPE(iface)), \
		.ptr = (&(TASK_IFACE_ARGS_TYPE(iface)) { __VA_ARGS__ }) \
	})

#define DEFINE_TASK_INTERFACE(iface, argstruct) \
	typedef TASK_ARGS_STRUCT(argstruct) TASK_IFACE_ARGS_TYPE(iface); \
	typedef struct { \
		TASK_IFACE_ARGS_TYPE(iface) *ptr; \
		size_t size; \
	} TASK_IFACE_ARGS_SIZED_PTR_TYPE(iface); \
	typedef struct { \
		CoTaskFunc _cotask_##iface##_thunk; \
	} TASK_INDIRECT_TYPE(iface)  /* require semicolon */

#define DEFINE_TASK_INTERFACE_WITH_BASE(iface, ibase, argstruct) \
	typedef struct { \
		TASK_IFACE_ARGS_TYPE(ibase) base; \
		TASK_ARGS_STRUCT(argstruct); \
	} TASK_IFACE_ARGS_TYPE(iface); \
	typedef struct { \
		union { \
			TASK_IFACE_ARGS_SIZED_PTR_TYPE(ibase) base; \
			struct { \
				TASK_IFACE_ARGS_TYPE(iface) *ptr; \
				size_t size; \
			}; \
		}; \
	} TASK_IFACE_ARGS_SIZED_PTR_TYPE(iface); \
	typedef struct { \
		union { \
			TASK_INDIRECT_TYPE(ibase) base; \
			CoTaskFunc _cotask_##iface##_thunk; \
			CoTaskFunc _cotask_##ibase##_thunk; \
		}; \
	} TASK_INDIRECT_TYPE(iface)  /* require semicolon */\

#define TASK_INDIRECT_TYPE_ALIAS(task) TASK_IFACE_NAME(task, HANDLEALIAS)

#define ARGS (*_cotask_args)

#define NO_ARGS attr_deprecated("Use { } instead of NO_ARGS") { }

// NOTE: the nested anonymous struct hack allows us to support both of these syntaxes:
//       INVOKE_TASK(foo, ENT_BOX(bar));
//       INVOKE_TASK(foo, { ENT_BOX(bar) });
#define TASK_ARGS_STRUCT(argstruct) struct { struct argstruct; }

#define TASK_COMMON_PRIVATE_DECLARATIONS(name) \
	/* user-defined task body */ \
	static void COTASK_##name(TASK_ARGS_TYPE(name) *_cotask_args) /* require semicolon */

#define TASK_COMMON_DECLARATIONS(name, argstype, handletype, linkage) \
	/* produce warning if the task is never used */ \
	linkage char COTASK_UNUSED_CHECK_##name; \
	/* type of indirect handle to a compatible task */ \
	typedef handletype TASK_INDIRECT_TYPE_ALIAS(name); \
	/* user-defined type of args struct */ \
	typedef argstype TASK_ARGS_TYPE(name); \
	/* type of internal args struct for INVOKE_TASK_DELAYED */ \
	struct TASK_ARGSDELAY_NAME(name) { \
		int delay; \
		/* NOTE: this must be last for interface inheritance to work! */ \
		TASK_ARGS_TYPE(name) real_args; \
	}; \
	/* type of internal args struct for INVOKE_TASK_WHEN */ \
	struct TASK_ARGSCOND_NAME(name) { \
		CoEvent *event; \
		bool unconditional; \
		/* NOTE: this must be last for interface inheritance to work! */ \
		TASK_ARGS_TYPE(name) real_args; \
	}; \
	/* task entry point for INVOKE_TASK */ \
	attr_unused linkage void *COTASKTHUNK_##name(void *arg, size_t arg_size); \
	/* task entry point for INVOKE_TASK_DELAYED */ \
	attr_unused linkage void *COTASKTHUNKDELAY_##name(void *arg, size_t arg_size); \
	/* task entry point for INVOKE_TASK_WHEN and INVOKE_TASK_AFTER */ \
	attr_unused linkage void *COTASKTHUNKCOND_##name(void *arg, size_t arg_size) /* require semicolon */ \

#define TASK_COMMON_THUNK_DEFINITIONS(name, linkage) \
	/* task entry point for INVOKE_TASK */ \
	attr_unused linkage void *COTASKTHUNK_##name(void *arg, size_t arg_size) { \
		/* copy args to our coroutine stack so that they're valid after caller returns */ \
		TASK_ARGS_TYPE(name) args_copy = { }; \
		assume(sizeof(args_copy) >= arg_size); \
		memcpy(&args_copy, arg, arg_size); \
		/* call body */ \
		COTASK_##name(&args_copy); \
		/* exit coroutine */ \
		return NULL; \
	} \
	/* task entry point for INVOKE_TASK_DELAYED */ \
	attr_unused linkage void *COTASKTHUNKDELAY_##name(void *arg, size_t arg_size) { \
		/* copy args to our coroutine stack so that they're valid after caller returns */ \
		TASK_ARGSDELAY(name) args_copy = { }; \
		assume(sizeof(args_copy) >= arg_size); \
		memcpy(&args_copy, arg, arg_size); \
		/* if delay is negative, bail out early */ \
		if(args_copy.delay < 0) return NULL; \
		/* wait out the delay */ \
		WAIT(args_copy.delay); \
		/* call body */ \
		COTASK_##name(&args_copy.real_args); \
		/* exit coroutine */ \
		return NULL; \
	} \
	/* task entry point for INVOKE_TASK_WHEN and INVOKE_TASK_AFTER */ \
	attr_unused linkage void *COTASKTHUNKCOND_##name(void *arg, size_t arg_size) { \
		/* copy args to our coroutine stack so that they're valid after caller returns */ \
		TASK_ARGSCOND(name) args_copy = { }; \
		assume(sizeof(args_copy) >= arg_size); \
		memcpy(&args_copy, arg, arg_size); \
		/* wait for event, and if it wasn't canceled (or if we want to run unconditionally)... */ \
		if(WAIT_EVENT(args_copy.event).event_status == CO_EVENT_SIGNALED || args_copy.unconditional) { \
			/* call body */ \
			COTASK_##name(&args_copy.real_args); \
		} \
		/* exit coroutine */ \
		return NULL; \
	}

#define TASK_COMMON_BEGIN_BODY_DEFINITION(name, linkage) \
	linkage void COTASK_##name(TASK_ARGS_TYPE(name) *_cotask_args)


#define DECLARE_TASK_EXPLICIT(name, argstype, handletype, linkage) \
	TASK_COMMON_DECLARATIONS(name, argstype, handletype, linkage) /* require semicolon */

#define DEFINE_TASK_EXPLICIT(name, linkage) \
	TASK_COMMON_PRIVATE_DECLARATIONS(name); \
	TASK_COMMON_THUNK_DEFINITIONS(name, linkage) \
	/* begin task body definition */ \
	TASK_COMMON_BEGIN_BODY_DEFINITION(name, linkage)


/* declare a task with static linkage (needs to be defined later) */
#define DECLARE_TASK(name, argstruct) \
	DECLARE_TASK_EXPLICIT(name, TASK_ARGS_STRUCT(argstruct), void, static) /* require semicolon */

/* declare a task with static linkage that conforms to a common interface (needs to be defined later) */
#define DECLARE_TASK_WITH_INTERFACE(name, iface) \
	DECLARE_TASK_EXPLICIT(name, TASK_IFACE_ARGS_TYPE(iface), TASK_INDIRECT_TYPE(iface), static) /* require semicolon */

/* define a task with static linkage (needs to be declared first) */
#define DEFINE_TASK(name) \
	DEFINE_TASK_EXPLICIT(name, static)

/* declare and define a task with static linkage */
#define TASK(name, argstruct) \
	DECLARE_TASK(name, argstruct); \
	DEFINE_TASK(name)

/* declare and define a task with static linkage that conforms to a common interface */
#define TASK_WITH_INTERFACE(name, iface) \
	DECLARE_TASK_WITH_INTERFACE(name, iface); \
	DEFINE_TASK(name)


/* declare a task with extern linkage (needs to be defined later) */
#define DECLARE_EXTERN_TASK(name, argstruct) \
	DECLARE_TASK_EXPLICIT(name, TASK_ARGS_STRUCT(argstruct), void, extern) /* require semicolon */

/* declare a task with extern linkage that conforms to a common interface (needs to be defined later) */
#define DECLARE_EXTERN_TASK_WITH_INTERFACE(name, iface) \
	DECLARE_TASK_EXPLICIT(name, TASK_IFACE_ARGS_TYPE(iface), TASK_INDIRECT_TYPE(iface), extern) /* require semicolon */

/* define a task with extern linkage (needs to be declared first) */
#define DEFINE_EXTERN_TASK(name) \
	char COTASK_UNUSED_CHECK_##name; \
	DEFINE_TASK_EXPLICIT(name, extern)

/*
 * INVOKE_TASK(task_name, args...)
 * INVOKE_SUBTASK(task_name, args...)
 *
 * This is the most basic way to start an asynchronous task. Control is transferred
 * to the new task immediately when this is called, and returns to the call site
 * when the task yields or terminates.
 *
 * Args are optional. They are treated simply as an initializer for the task's
 * args struct, so it's possible to use designated initializer syntax to emulate
 * "keyword arguments", etc.
 *
 * INVOKE_SUBTASK is identical INVOKE_TASK, except the spawned task will attach
 * to the currently executing task, becoming its "sub-task" or "slave". When a
 * task finishes executing, all of its sub-tasks are also terminated recursively.
 *
 * Other INVOKE_ macros with a _SUBTASK version behave analogously.
 */

#define INVOKE_TASK(_task, ...) \
	_internal_INVOKE_TASK(cosched_new_task, _task, ##__VA_ARGS__)
#define INVOKE_SUBTASK(_task, ...) \
	_internal_INVOKE_TASK(cosched_new_subtask, _task, ##__VA_ARGS__)

#define _internal_INVOKE_TASK(task_constructor, name, ...) ( \
	(void)COTASK_UNUSED_CHECK_##name, \
	task_constructor( \
		_cosched_global, \
		COTASKTHUNK_##name, \
		(&(TASK_ARGS_TYPE(name)) { __VA_ARGS__ }), \
		sizeof(TASK_ARGS_TYPE(name)), \
		#name \
	) \
)

/*
 * INVOKE_TASK_DELAYED(delay, task_name, args...)
 * INVOKE_SUBTASK_DELAYED(delay, task_name, args...)
 *
 * Like INVOKE_TASK, but the task will yield <delay> times before executing the
 * actual task body.
 *
 * If <delay> is negative, the task will not be invoked. The arguments are still
 * evaluated, however. (Caveat: in the current implementation, a task is spawned
 * either way; it just aborts early without executing the body if the delay is
 * negative, so there's some overhead).
 */

#define INVOKE_TASK_DELAYED(_delay, _task, ...) \
	_internal_INVOKE_TASK_DELAYED(cosched_new_task, _delay, _task, ##__VA_ARGS__)
#define INVOKE_SUBTASK_DELAYED(_delay, _task, ...) \
	_internal_INVOKE_TASK_DELAYED(cosched_new_subtask, _delay, _task, ##__VA_ARGS__)

#define _internal_INVOKE_TASK_DELAYED(task_constructor, _delay, name, ...) ( \
	(void)COTASK_UNUSED_CHECK_##name, \
	task_constructor( \
		_cosched_global, \
		COTASKTHUNKDELAY_##name, \
		(&(TASK_ARGSDELAY(name)) { \
			.real_args = { __VA_ARGS__ }, \
			.delay = (_delay) \
		}), \
		sizeof(TASK_ARGSDELAY(name)), \
		#name \
	) \
)

/*
 * INVOKE_TASK_WHEN(event, task_name, args...)
 * INVOKE_SUBTASK_WHEN(event, task_name, args...)
 *
 * INVOKE_TASK_AFTER(event, task_name, args...)
 * INVOKE_SUBTASK_AFTER(event, task_name, args...)
 *
 * Both INVOKE_TASK_WHEN and INVOKE_TASK_AFTER spawn a task that waits for an
 * event to occur. The difference is that _WHEN aborts the task if the event has
 * been canceled, but _AFTER proceeds to execute it unconditionally.
 *
 * <event> is a pointer to a CoEvent struct.
 */

#define INVOKE_TASK_WHEN(_event, _task, ...) \
	_internal_INVOKE_TASK_ON_EVENT(cosched_new_task, false, _event, _task, ##__VA_ARGS__)
#define INVOKE_SUBTASK_WHEN(_event, _task, ...) \
	_internal_INVOKE_TASK_ON_EVENT(cosched_new_subtask, false, _event, _task, ##__VA_ARGS__)

#define INVOKE_TASK_AFTER(_event, _task, ...) \
	_internal_INVOKE_TASK_ON_EVENT(cosched_new_task, true, _event, _task, ## __VA_ARGS__)
#define INVOKE_SUBTASK_AFTER(_event, _task, ...) \
	_internal_INVOKE_TASK_ON_EVENT(cosched_new_subtask, true, _event, _task, ## __VA_ARGS__)

#define _internal_INVOKE_TASK_ON_EVENT(task_constructor, is_unconditional, _event, name, ...) ( \
	(void)COTASK_UNUSED_CHECK_##name, \
	task_constructor( \
		_cosched_global, \
		COTASKTHUNKCOND_##name, \
		(&(TASK_ARGSCOND(name)) { \
			.real_args = { __VA_ARGS__ }, \
			.event = (_event), \
			.unconditional = is_unconditional \
		}), \
		sizeof(TASK_ARGSCOND(name)), \
		#name \
	) \
)

/*
 * CANCEL_TASK_WHEN(event, boxed_task)
 * CANCEL_TASK_AFTER(event, boxed_task)
 *
 * Invokes an auxiliary task that will wait for an event, and then cancel another
 * running task. The difference between WHEN and AFTER is the same as in
 * INVOKE_TASK_WHEN/INVOKE_TASK_AFTER -- this is a simple wrapper around those.
 *
 * <event> is a pointer to a CoEvent struct.
 * <boxed_task> is a BoxedTask struct; use cotask_box to obtain one from a pointer.
 * You can also use the THIS_TASK macro to refer to the currently running task.
 */

#define CANCEL_TASK_WHEN(_event, _task) INVOKE_TASK_WHEN(_event, _cancel_task_helper, _task)
#define CANCEL_TASK_AFTER(_event, _task) INVOKE_TASK_AFTER(_event, _cancel_task_helper, _task)

DECLARE_EXTERN_TASK(_cancel_task_helper, { BoxedTask task; });

#define CANCEL_TASK(boxed_task) cotask_cancel(cotask_unbox(boxed_task))

#define TASK_INDIRECT(iface, task) ( \
	(void)COTASK_UNUSED_CHECK_##task, \
	(TASK_INDIRECT_TYPE_ALIAS(task)) { ._cotask_##iface##_thunk = COTASKTHUNK_##task } \
)

#define TASK_INDIRECT_INIT(iface, task) \
	{ ._cotask_##iface##_thunk = COTASKTHUNK_##task } \

#define INVOKE_TASK_INDIRECT_(task_constructor, iface, taskhandle, ...) ( \
	task_constructor( \
		_cosched_global, \
		taskhandle._cotask_##iface##_thunk, \
		(&(TASK_IFACE_ARGS_TYPE(iface)) { __VA_ARGS__ }), \
		sizeof(TASK_IFACE_ARGS_TYPE(iface)), \
		"<indirect:"#iface">" \
	) \
)

#define INVOKE_TASK_INDIRECT(_iface, _handle, ...) \
	INVOKE_TASK_INDIRECT_(cosched_new_task, _iface, _handle, ##__VA_ARGS__)
#define INVOKE_SUBTASK_INDIRECT(_iface, _handle, ...) \
	INVOKE_TASK_INDIRECT_(cosched_new_subtask, iface, _handle, ##__VA_ARGS__)

#define THIS_TASK         cotask_box(cotask_active())
#define TASK_EVENTS(task) cotask_get_events(cotask_unbox(task))
#define TASK_MALLOC(size) cotask_malloc(cotask_active(), size)

#define TASK_HOST_ENT(ent_struct_type) \
	ENT_CAST(cotask_host_entity(cotask_active(), sizeof(ent_struct_type), ENT_TYPE_ID(ent_struct_type)), ent_struct_type)

#define TASK_HOST_EVENTS(events_array) \
	cotask_host_events(cotask_active(), sizeof(events_array)/sizeof(CoEvent), &((events_array)._first_event_))

#define YIELD                cotask_yield(NULL)
#define WAIT(delay)          cotask_wait(delay)
#define WAIT_EVENT(e)        cotask_wait_event(e)
#define WAIT_EVENT_OR_DIE(e) cotask_wait_event_or_die(e)
#define WAIT_EVENT_ONCE(e)   cotask_wait_event_once(e)
#define STALL                cotask_wait(INT_MAX)
#define AWAIT_SUBTASKS       cotask_wait_subtasks()

// first arg of the generated function needs to be the ent, because ENT_UNBOXED_DISPATCH_FUNCTION dispatches on first arg.
#define _cotask_emit_bindfunc(typename, ...) \
	INLINE typename *_cotask_bind_to_entity_##typename(typename *ent, CoTask *task) { \
		return ENT_CAST((cotask_bind_to_entity)( \
			task, \
			ent ? UNION_CAST(typename*, EntityInterface*, ent) : NULL), \
			typename \
		); \
	}

ENTITIES(_cotask_emit_bindfunc,)
#undef _cotask_emit_bindfunc

INLINE EntityInterface *_cotask_bind_to_entity_Entity(EntityInterface *ent, CoTask *task) {
	return (cotask_bind_to_entity)(task, ent);
}

#define cotask_bind_to_entity(task, ent) \
	ENT_UNBOXED_DISPATCH_FUNCTION(_cotask_bind_to_entity_, ent, task)

#define TASK_BIND(ent_or_box) cotask_bind_to_entity(cotask_active(), ENT_UNBOX_OR_PASSTHROUGH(ent_or_box))

#endif // IGUARD_coroutine_h
