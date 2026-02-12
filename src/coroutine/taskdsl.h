/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2024, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2024, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#include "entity.h"

#include "cotask.h"
#include "coevent.h"
#include "cosched.h"

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
#define DECLARE_TASK(name, ...) \
	MACROHAX_OVERLOAD_HASARGS(DECLARE_TASK_, __VA_ARGS__)(name, ##__VA_ARGS__)
#define DECLARE_TASK_1(name, ...) \
	DECLARE_TASK_EXPLICIT(name, TASK_ARGS_STRUCT(__VA_ARGS__), void, static) /* require semicolon */
#define DECLARE_TASK_0(name) DECLARE_TASK_1(name, { })

/* declare a task with static linkage that conforms to a common interface (needs to be defined later) */
#define DECLARE_TASK_WITH_INTERFACE(name, iface) \
	DECLARE_TASK_EXPLICIT(name, TASK_IFACE_ARGS_TYPE(iface), TASK_INDIRECT_TYPE(iface), static) /* require semicolon */

/* define a task with static linkage (needs to be declared first) */
#define DEFINE_TASK(name) \
	DEFINE_TASK_EXPLICIT(name, static)

/* declare and define a task with static linkage */
#define TASK(name, ...) \
	DECLARE_TASK(name, ##__VA_ARGS__); \
	DEFINE_TASK(name)

/* declare and define a task with static linkage that conforms to a common interface */
#define TASK_WITH_INTERFACE(name, iface) \
	DECLARE_TASK_WITH_INTERFACE(name, iface); \
	DEFINE_TASK(name)


/* declare a task with extern linkage (needs to be defined later) */
#define DECLARE_EXTERN_TASK(name, ...)\
	MACROHAX_OVERLOAD_HASARGS(DECLARE_EXTERN_TASK_, __VA_ARGS__)(name, ##__VA_ARGS__)
#define DECLARE_EXTERN_TASK_1(name, ...) \
	DECLARE_TASK_EXPLICIT(name, TASK_ARGS_STRUCT(__VA_ARGS__), void, extern) /* require semicolon */
#define DECLARE_EXTERN_TASK_0(name) \
	DECLARE_EXTERN_TASK_1(name, { })

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
	_internal_INVOKE_TASK(THIS_SCHED, cosched_new_task, _task, ##__VA_ARGS__)
#define INVOKE_SUBTASK(_task, ...) \
	_internal_INVOKE_TASK(THIS_SCHED, cosched_new_subtask, _task, ##__VA_ARGS__)
#define SCHED_INVOKE_TASK(_sched, _task, ...) \
	_internal_INVOKE_TASK(_sched, cosched_new_task, _task, ##__VA_ARGS__)

#define _internal_INVOKE_TASK(sched, task_constructor, name, ...) ( \
	(void)COTASK_UNUSED_CHECK_##name, \
	task_constructor( \
		sched, \
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
	_internal_INVOKE_TASK_DELAYED(THIS_SCHED, cosched_new_task, _delay, _task, ##__VA_ARGS__)
#define INVOKE_SUBTASK_DELAYED(_delay, _task, ...) \
	_internal_INVOKE_TASK_DELAYED(THIS_SCHED, cosched_new_subtask, _delay, _task, ##__VA_ARGS__)
#define SCHED_INVOKE_TASK_DELAYED(_sched, _delay, _task, ...) \
	_internal_INVOKE_TASK_DELAYED(_sched, cosched_new_task, _delay, _task, ##__VA_ARGS__)

#define _internal_INVOKE_TASK_DELAYED(sched, task_constructor, _delay, name, ...) ( \
	(void)COTASK_UNUSED_CHECK_##name, \
	task_constructor( \
		sched, \
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
	_internal_INVOKE_TASK_ON_EVENT(THIS_SCHED, cosched_new_task, false, _event, _task, ##__VA_ARGS__)
#define INVOKE_SUBTASK_WHEN(_event, _task, ...) \
	_internal_INVOKE_TASK_ON_EVENT(THIS_SCHED, cosched_new_subtask, false, _event, _task, ##__VA_ARGS__)
#define SCHED_INVOKE_TASK_WHEN(_sched, _event, _task, ...) \
	_internal_INVOKE_TASK_ON_EVENT(_sched, cosched_new_task, false, _event, _task, ##__VA_ARGS__)

#define INVOKE_TASK_AFTER(_event, _task, ...) \
	_internal_INVOKE_TASK_ON_EVENT(THIS_SCHED, cosched_new_task, true, _event, _task, ## __VA_ARGS__)
#define INVOKE_SUBTASK_AFTER(_event, _task, ...) \
	_internal_INVOKE_TASK_ON_EVENT(THIS_SCHED, cosched_new_subtask, true, _event, _task, ## __VA_ARGS__)
#define SCHED_INVOKE_TASK_AFTER(_sched, _event, _task, ...) \
	_internal_INVOKE_TASK_ON_EVENT(_sched, cosched_new_task, true, _event, _task, ##__VA_ARGS__)

#define _internal_INVOKE_TASK_ON_EVENT(sched, task_constructor, is_unconditional, _event, name, ...) ( \
	(void)COTASK_UNUSED_CHECK_##name, \
	task_constructor( \
		sched, \
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

#define INVOKE_TASK_INDIRECT_(sched, task_constructor, iface, taskhandle, ...) ( \
	task_constructor( \
		sched, \
		taskhandle._cotask_##iface##_thunk, \
		(&(TASK_IFACE_ARGS_TYPE(iface)) { __VA_ARGS__ }), \
		sizeof(TASK_IFACE_ARGS_TYPE(iface)), \
		"<indirect:"#iface">" \
	) \
)

#define SCHED_INVOKE_TASK_INDIRECT(_sched, _iface, _handle, ...) \
	INVOKE_TASK_INDIRECT_(_sched, cosched_new_task, _iface, _handle, ##__VA_ARGS__)
#define INVOKE_TASK_INDIRECT(_iface, _handle, ...) \
	INVOKE_TASK_INDIRECT_(THIS_SCHED, cosched_new_task, _iface, _handle, ##__VA_ARGS__)
#define INVOKE_SUBTASK_INDIRECT(_iface, _handle, ...) \
	INVOKE_TASK_INDIRECT_(THIS_SCHED, cosched_new_subtask, iface, _handle, ##__VA_ARGS__)

#define THIS_TASK         cotask_box(cotask_active_unsafe())
#define TASK_EVENTS(task) cotask_get_events(cotask_unbox(task))
#define TASK_MALLOC(size) cotask_malloc(cotask_active_unsafe(), size)

#define THIS_SCHED        cotask_get_sched(cotask_active_unsafe())

#define TASK_HOST_ENT(ent_struct_type) \
	ENT_CAST(cotask_host_entity(cotask_active_unsafe(), sizeof(ent_struct_type), ENT_TYPE_ID(ent_struct_type)), ent_struct_type)

#define TASK_HOST_EVENTS(events_array) ({ \
	(events_array) = cotask_malloc(cotask_active_unsafe(), sizeof(*(events_array))); \
	cotask_host_events(cotask_active_unsafe(), sizeof(*events_array)/sizeof(CoEvent), &((events_array)->_first_event_)); \
})

#define YIELD                cotask_yield(NULL)
#define WAIT(delay)          cotask_wait(delay)
#define WAIT_EVENT(e)        cotask_wait_event(e)
#define WAIT_EVENT_OR_DIE(e) cotask_wait_event_or_die(e)
#define WAIT_EVENT_ONCE(e)   cotask_wait_event_once(e)
#define STALL                cotask_wait(INT_MAX)
#define AWAIT_SUBTASKS       cotask_wait_subtasks()

#define NOT_NULL_OR_DIE(expr) ({ \
	auto _not_null_ptr = (expr); \
	if(_not_null_ptr == NULL) { \
		cotask_cancel(cotask_active_unsafe()); \
		UNREACHABLE; \
	} \
	NOT_NULL(_not_null_ptr); \
})

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

#define TASK_BIND(ent_or_box) \
	cotask_bind_to_entity(cotask_active_unsafe(), ENT_UNBOX_OR_PASSTHROUGH(ent_or_box))
