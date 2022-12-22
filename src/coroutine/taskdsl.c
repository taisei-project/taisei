
#include "taskdsl.h"

DEFINE_EXTERN_TASK(_cancel_task_helper) {
	CoTask *task = cotask_unbox(ARGS.task);

	if(task) {
		cotask_cancel(task);
	}
}
