
// Fill the global dialog_tasks_<character> struct

#define WITH_EVENTS(_name, _events) WITHOUT_EVENTS(_name)
#define WITHOUT_EVENTS(_name) \
	._name = MACROHAX_EXPAND(\
		MACROHAX_DEFER(TASK_INDIRECT_INIT)( \
			_name##Dialog, MACROHAX_CONCAT(EXPORT_DIALOG_TASKS_CHARACTER, _##_name##Dialog) \
		) \
	),

PlayerDialogTasks MACROHAX_CONCAT(dialog_tasks_, EXPORT_DIALOG_TASKS_CHARACTER) = {
	DIALOG_SCRIPTS
};

// Let the compiler know that the tasks are "used". TASK_INDIRECT_INIT can't do that.

#undef WITHOUT_EVENTS

#define WITHOUT_EVENTS(_name) \
	attr_unused static char MACROHAX_CONCAT( \
		COTASK_UNUSED_CHECK_, MACROHAX_CONCAT(EXPORT_DIALOG_TASKS_CHARACTER, _##_name##Dialog) \
	);

DIALOG_SCRIPTS

#undef WITH_EVENTS
#undef WITHOUT_EVENTS

#undef EXPORT_DIALOG_TASKS_CHARACTER
