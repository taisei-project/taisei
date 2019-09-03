/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#ifndef IGUARD_coroutine_h
#define IGUARD_coroutine_h

#include "taisei.h"

#include "entity.h"
#include <koishi.h>

typedef struct CoTask CoTask;
typedef struct CoSched CoSched;
typedef void *(*CoTaskFunc)(void *arg);

// target for the INVOKE_TASK macro
extern CoSched *_cosched_global;

typedef enum CoStatus {
	CO_STATUS_SUSPENDED = KOISHI_SUSPENDED,
	CO_STATUS_RUNNING   = KOISHI_RUNNING,
	CO_STATUS_DEAD      = KOISHI_DEAD,
} CoStatus;

typedef enum CoEventStatus {
	CO_EVENT_PENDING,
	CO_EVENT_SIGNALED,
	CO_EVENT_CANCELED,
} CoEventStatus;

typedef struct BoxedTask {
	uintptr_t ptr;
	uint32_t unique_id;
} BoxedTask;

typedef struct CoEvent {
	// ListAnchor subscribers;
	// FIXME: Is there a better way than a dynamic array?
	// An intrusive linked list just isn't robust enough.
	BoxedTask *subscribers;
	uint32_t unique_id;
	uint16_t num_signaled;
	uint8_t num_subscribers;
	uint8_t num_subscribers_allocated;
} CoEvent;

struct CoSched {
	LIST_ANCHOR(CoTask) tasks, pending_tasks;
};

typedef struct CoWaitResult {
	int frames;
	CoEventStatus event_status;
} CoWaitResult;

void coroutines_init(void);
void coroutines_shutdown(void);

CoTask *cotask_new(CoTaskFunc func);
void cotask_free(CoTask *task);
bool cotask_cancel(CoTask *task);
void *cotask_resume(CoTask *task, void *arg);
void *cotask_yield(void *arg);
CoWaitResult cotask_wait_event(CoEvent *evt, void *arg);
CoStatus cotask_status(CoTask *task);
CoTask *cotask_active(void);
EntityInterface *cotask_bind_to_entity(CoTask *task, EntityInterface *ent) attr_returns_nonnull;
void cotask_set_finalizer(CoTask *task, CoTaskFunc finalizer, void *arg);
void cotask_enslave(CoTask *slave);

BoxedTask cotask_box(CoTask *task);
CoTask *cotask_unbox(BoxedTask box);

void coevent_init(CoEvent *evt);
void coevent_signal(CoEvent *evt);
void coevent_signal_once(CoEvent *evt);
void coevent_cancel(CoEvent *evt);

void _coevent_array_action(uint num, CoEvent *events, void (*func)(CoEvent*));

#define COEVENTS_ARRAY(...) union { CoEvent _first_event_; struct { CoEvent __VA_ARGS__; }; }

#define COEVENT_ARRAY_ACTION(func, array) (_coevent_array_action(sizeof(array)/sizeof(CoEvent), &((array)._first_event_), func))
#define COEVENT_INIT_ARRAY(array) COEVENT_ARRAY_ACTION(coevent_init, array)
#define COEVENT_CANCEL_ARRAY(array) COEVENT_ARRAY_ACTION(coevent_cancel, array)

void cosched_init(CoSched *sched);
CoTask *cosched_new_task(CoSched *sched, CoTaskFunc func, void *arg);  // creates and runs the task, schedules it for resume on cosched_run_tasks if it's still alive
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
#define TASK_INDIRECT_TYPE(iface) TASK_IFACE_NAME(iface, HANDLE)

#define DEFINE_TASK_INTERFACE(iface, argstruct) \
	typedef TASK_ARGS_STRUCT(argstruct) TASK_IFACE_ARGS_TYPE(iface); \
	typedef struct { void *(*_cotask_##iface##_thunk)(void*); } TASK_INDIRECT_TYPE(iface)  /* require semicolon */

#define TASK_INDIRECT_TYPE_ALIAS(task) TASK_IFACE_NAME(task, HANDLEALIAS)

#define ARGS (*_cotask_args)

#define NO_ARGS { char _dummy_0; }
#define TASK_ARGS_STRUCT(argstruct) struct { struct argstruct; char _dummy_1; }

#define TASK_COMMON_PRIVATE_DECLARATIONS(name) \
	/* user-defined task body */ \
	static void COTASK_##name(TASK_ARGS_TYPE(name) *_cotask_args); \
	/* called from the entry points before task body (inlined, hopefully) */ \
	INLINE void COTASKPROLOGUE_##name(TASK_ARGS_TYPE(name) *_cotask_args) /* require semicolon */ \

#define TASK_COMMON_DECLARATIONS(name, argstype, handletype, linkage) \
	/* produce warning if the task is never used */ \
	linkage char COTASK_UNUSED_CHECK_##name; \
	/* type of indirect handle to a compatible task */ \
	typedef handletype TASK_INDIRECT_TYPE_ALIAS(name); \
	/* user-defined type of args struct */ \
	typedef argstype TASK_ARGS_TYPE(name); \
	/* type of internal args struct for INVOKE_TASK_DELAYED */ \
	struct TASK_ARGSDELAY_NAME(name) { TASK_ARGS_TYPE(name) real_args; int delay; }; \
	/* type of internal args struct for INVOKE_TASK_WHEN */ \
	struct TASK_ARGSCOND_NAME(name) { TASK_ARGS_TYPE(name) real_args; CoEvent *event; }; \
	/* task entry point for INVOKE_TASK */ \
	attr_unused linkage void *COTASKTHUNK_##name(void *arg); \
	/* task entry point for INVOKE_TASK_DELAYED */ \
	attr_unused linkage void *COTASKTHUNKDELAY_##name(void *arg); \
	/* task entry point for INVOKE_TASK_WHEN */ \
	attr_unused linkage void *COTASKTHUNKCOND_##name(void *arg) /* require semicolon */ \

#define TASK_COMMON_THUNK_DEFINITIONS(name, linkage) \
	/* task entry point for INVOKE_TASK */ \
	attr_unused linkage void *COTASKTHUNK_##name(void *arg) { \
		/* copy args to our coroutine stack so that they're valid after caller returns */ \
		TASK_ARGS_TYPE(name) args_copy = *(TASK_ARGS_TYPE(name)*)arg; \
		/* call prologue */ \
		COTASKPROLOGUE_##name(&args_copy); \
		/* call body */ \
		COTASK_##name(&args_copy); \
		/* exit coroutine */ \
		return NULL; \
	} \
	/* task entry point for INVOKE_TASK_DELAYED */ \
	attr_unused linkage void *COTASKTHUNKDELAY_##name(void *arg) { \
		/* copy args to our coroutine stack so that they're valid after caller returns */ \
		TASK_ARGSDELAY(name) args_copy = *(TASK_ARGSDELAY(name)*)arg; \
		/* wait out the delay */ \
		WAIT(args_copy.delay); \
		/* call prologue */ \
		COTASKPROLOGUE_##name(&args_copy.real_args); \
		/* call body */ \
		COTASK_##name(&args_copy.real_args); \
		/* exit coroutine */ \
		return NULL; \
	} \
	/* task entry point for INVOKE_TASK_WHEN */ \
	attr_unused linkage void *COTASKTHUNKCOND_##name(void *arg) { \
		/* copy args to our coroutine stack so that they're valid after caller returns */ \
		TASK_ARGSCOND(name) args_copy = *(TASK_ARGSCOND(name)*)arg; \
		/* wait for event, and if it wasn't canceled... */ \
		if(WAIT_EVENT(args_copy.event).event_status == CO_EVENT_SIGNALED) { \
			/* call prologue */ \
			COTASKPROLOGUE_##name(&args_copy.real_args); \
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
	/* empty prologue */ \
	INLINE void COTASKPROLOGUE_##name(TASK_ARGS_TYPE(name) *_cotask_args) { } \
	/* begin task body definition */ \
	TASK_COMMON_BEGIN_BODY_DEFINITION(name, linkage)


/* declare a task with static linkage (needs to be defined later) */
#define DECLARE_TASK(name, argstruct) \
	DECLARE_TASK_EXPLICIT(name, TASK_ARGS_STRUCT(argstruct), void, static) /* require semicolon */

/* TODO document */
#define DECLARE_TASK_WITH_INTERFACE(name, iface) \
	DECLARE_TASK_EXPLICIT(name, TASK_IFACE_ARGS_TYPE(iface), TASK_INDIRECT_TYPE(iface), static) /* require semicolon */

/* define a task with static linkage (needs to be declared first) */
#define DEFINE_TASK(name) \
	DEFINE_TASK_EXPLICIT(name, static)

/* declare and define a task with static linkage */
#define TASK(name, argstruct) \
	DECLARE_TASK(name, argstruct); \
	DEFINE_TASK(name)

/* TODO document */
#define TASK_WITH_INTERFACE(name, iface) \
	DECLARE_TASK_WITH_INTERFACE(name, iface); \
	DEFINE_TASK(name)


/* declare a task with extern linkage (needs to be defined later) */
#define DECLARE_EXTERN_TASK(name, argstruct) \
	DECLARE_TASK_EXPLICIT(name, TASK_ARGS_STRUCT(argstruct), void, extern) /* require semicolon */

/* TODO document */
#define DECLARE_EXTERN_TASK_WITH_INTERFACE(name, iface) \
	DECLARE_TASK_EXPLICIT(name, TASK_IFACE_ARGS_TYPE(iface), TASK_INDIRECT_TYPE(iface), extern) /* require semicolon */

/* define a task with extern linkage (needs to be declared first) */
#define DEFINE_EXTERN_TASK(name) \
	DEFINE_TASK_EXPLICIT(name, extern)


#define DEFINE_TASK_WITH_FINALIZER_EXPLICIT(name, linkage) \
	TASK_COMMON_PRIVATE_DECLARATIONS(name); \
	TASK_COMMON_THUNK_DEFINITIONS(name, linkage) \
	/* error out if using TASK_FINALIZER without TASK_WITH_FINALIZER */ \
	struct COTASK__##name##__not_declared_using_TASK_WITH_FINALIZER { char dummy; }; \
	/* user-defined finalizer function */ \
	INLINE void COTASKFINALIZER_##name(TASK_ARGS_TYPE(name) *_cotask_args); \
	/* real finalizer entry point */ \
	static void *COTASKFINALIZERTHUNK_##name(void *arg) { \
		COTASKFINALIZER_##name((TASK_ARGS_TYPE(name)*)arg); \
		return NULL; \
	} \
	/* prologue; sets up finalizer before executing task body */ \
	INLINE void COTASKPROLOGUE_##name(TASK_ARGS_TYPE(name) *_cotask_args) { \
		cotask_set_finalizer(cotask_active(), COTASKFINALIZERTHUNK_##name, _cotask_args); \
	} \
	/* begin task body definition */ \
	TASK_COMMON_BEGIN_BODY_DEFINITION(name, linkage)

/* define a task that needs a finalizer with static linkage (needs to be declared first) */
#define DEFINE_TASK_WITH_FINALIZER(name) \
 	DEFINE_TASK_WITH_FINALIZER_EXPLICIT(name, static)

/* define a task that needs a finalizer with static linkage (needs to be declared first) */
#define DEFINE_EXTERN_TASK_WITH_FINALIZER(name) \
	DEFINE_TASK_WITH_FINALIZER_EXPLICIT(name, extern)

/* declare and define a task that needs a finalizer with static linkage */
#define TASK_WITH_FINALIZER(name, argstruct) \
	DECLARE_TASK(name, argstruct); \
	DEFINE_TASK_WITH_FINALIZER(name)

/* define the finalizer for a TASK_WITH_FINALIZER */
#define TASK_FINALIZER(name) \
	/* error out if using TASK_FINALIZER without TASK_WITH_FINALIZER */ \
	attr_unused struct COTASK__##name##__not_declared_using_TASK_WITH_FINALIZER COTASK__##name##__not_declared_using_TASK_WITH_FINALIZER; \
	/* begin finalizer body definition */ \
	static void COTASKFINALIZER_##name(TASK_ARGS_TYPE(name) *_cotask_args)


INLINE BoxedTask _cotask_invoke_helper(CoTask *t, bool is_subtask) {
	if(is_subtask) {
		cotask_enslave(t);
	}

	return cotask_box(t);
}


#define INVOKE_TASK_(is_subtask, name, ...) ( \
	(void)COTASK_UNUSED_CHECK_##name, \
	_cotask_invoke_helper(cosched_new_task(_cosched_global, COTASKTHUNK_##name, \
		&(TASK_ARGS_TYPE(name)) { __VA_ARGS__ } \
	), is_subtask) \
)

#define INVOKE_TASK(...) INVOKE_TASK_(false, __VA_ARGS__, ._dummy_1 = 0)
#define INVOKE_SUBTASK(...) INVOKE_TASK_(true, __VA_ARGS__, ._dummy_1 = 0)

#define INVOKE_TASK_WHEN_(is_subtask, _event, name, ...) ( \
	(void)COTASK_UNUSED_CHECK_##name, \
	_cotask_invoke_helper(cosched_new_task(_cosched_global, COTASKTHUNKCOND_##name, \
		&(TASK_ARGSCOND(name)) { .real_args = { __VA_ARGS__ }, .event = (_event) } \
	), is_subtask) \
)

#define INVOKE_TASK_WHEN(_event, ...) INVOKE_TASK_WHEN_(false, _event, __VA_ARGS__, ._dummy_1 = 0)
#define INVOKE_SUBTASK_WHEN(_event, ...) INVOKE_TASK_WHEN_(true, _event, __VA_ARGS__, ._dummy_1 = 0)

#define INVOKE_TASK_DELAYED_(is_subtask, _delay, name, ...) ( \
	(void)COTASK_UNUSED_CHECK_##name, \
	_cotask_invoke_helper(cosched_new_task(_cosched_global, COTASKTHUNKDELAY_##name, \
		&(TASK_ARGSDELAY(name)) { .real_args = { __VA_ARGS__ }, .delay = (_delay) } \
	), is_subtask) \
)

#define INVOKE_TASK_DELAYED(_delay, ...) INVOKE_TASK_DELAYED_(false, _delay, __VA_ARGS__, ._dummy_1 = 0)
#define INVOKE_SUBTASK_DELAYED(_delay, ...) INVOKE_TASK_DELAYED_(true, _delay, __VA_ARGS__, ._dummy_1 = 0)

DECLARE_EXTERN_TASK(_cancel_task_helper, { BoxedTask task; });

#define CANCEL_TASK_WHEN(_event, _task) INVOKE_TASK_WHEN(_event, _cancel_task_helper, _task)

#define TASK_INDIRECT(iface, task) ( \
	(void)COTASK_UNUSED_CHECK_##task, \
	(TASK_INDIRECT_TYPE_ALIAS(task)) { ._cotask_##iface##_thunk = COTASKTHUNK_##task } \
)

#define TASK_INDIRECT_INIT(iface, task) \
	{ ._cotask_##iface##_thunk = COTASKTHUNK_##task } \

#define INVOKE_TASK_INDIRECT_(is_subtask, iface, taskhandle, ...) ( \
	_cotask_invoke_helper(cosched_new_task(_cosched_global, taskhandle._cotask_##iface##_thunk, \
		&(TASK_IFACE_ARGS_TYPE(iface)) { __VA_ARGS__ } \
	), is_subtask) \
)

#define INVOKE_TASK_INDIRECT(iface, ...) INVOKE_TASK_INDIRECT_(false, iface, __VA_ARGS__, ._dummy_1 = 0)
#define INVOKE_SUBTASK_INDIRECT(iface, ...) INVOKE_TASK_INDIRECT_(true, iface, __VA_ARGS__, ._dummy_1 = 0)

#define THIS_TASK     cotask_box(cotask_active())

#define YIELD         cotask_yield(NULL)
#define WAIT(delay)   do { int _delay = (delay); while(_delay-- > 0) YIELD; } while(0)
#define WAIT_EVENT(e) cotask_wait_event((e), NULL)
#define STALL         do { YIELD; } while(1)

// to use these inside a coroutine, define a BREAK_CONDITION macro and a BREAK label.
#define CHECK_BREAK   do { if(BREAK_CONDITION) goto BREAK; } while(0)
#define BYIELD        do { YIELD; CHECK_BREAK; } while(0)
#define BWAIT(frames) do { WAIT(frames); CHECK_BREAK; } while(0)
#define BSTALL        do { BYIELD; } while(1)

#define ENT_TYPE(typename, id) \
	struct typename; \
	struct typename *_cotask_bind_to_entity_##typename(CoTask *task, struct typename *ent) attr_returns_nonnull attr_returns_max_aligned;

ENT_TYPES
#undef ENT_TYPE

#define cotask_bind_to_entity(task, ent) (_Generic((ent), \
	struct Projectile*: _cotask_bind_to_entity_Projectile, \
	struct Laser*: _cotask_bind_to_entity_Laser, \
	struct Enemy*: _cotask_bind_to_entity_Enemy, \
	struct Boss*: _cotask_bind_to_entity_Boss, \
	struct Player*: _cotask_bind_to_entity_Player, \
	struct Item*: _cotask_bind_to_entity_Item, \
	EntityInterface*: cotask_bind_to_entity \
)(task, ent))

#define TASK_BIND(box) cotask_bind_to_entity(cotask_active(), ENT_UNBOX(box))
#define TASK_BIND_UNBOXED(ent) cotask_bind_to_entity(cotask_active(), ent)

#endif // IGUARD_coroutine_h
