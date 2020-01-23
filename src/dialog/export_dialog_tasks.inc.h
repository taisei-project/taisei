
// this needs an extra indirection to expand EXPORT_DIALOG_TASKS_CHARACTER
#define EDT_CONCAT(a, b) MACROHAX_CONCAT(a, b)

// Fill the global dialog_tasks_<character> struct

#define WITH_EVENTS(_name, _events) WITHOUT_EVENTS(_name)
#define WITHOUT_EVENTS(_name) \
._name = MACROHAX_EXPAND(MACROHAX_DEFER(TASK_INDIRECT_INIT)(_name##Dialog, EDT_CONCAT(EXPORT_DIALOG_TASKS_CHARACTER, _##_name##Dialog))),

PlayerDialogTasks EDT_CONCAT(dialog_tasks_, EXPORT_DIALOG_TASKS_CHARACTER) = {
	DIALOG_SCRIPTS
};

// Let the compiler know that the tasks are "used". TASK_INDIRECT_INIT can't do that.

#undef WITHOUT_EVENTS

#define WITHOUT_EVENTS(_name) \
	attr_unused static char MACROHAX_EXPAND(MACROHAX_DEFER(EDT_CONCAT)(COTASK_UNUSED_CHECK_, EDT_CONCAT(EXPORT_DIALOG_TASKS_CHARACTER, _##_name##Dialog)));

DIALOG_SCRIPTS

#undef WITH_EVENTS
#undef WITHOUT_EVENTS
#undef EDT_CONTAT

#undef EXPORT_DIALOG_TASKS_CHARACTER
