
#include "public.h"

void vfs_sync(VFSSyncMode mode, CallChain next) {
	run_call_chain(&next, NULL);
}
