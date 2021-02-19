/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "taisei.h"

#include "global.h"
#include "refs.h"

/*
 * NOTE: if you're here to attempt fixing any of this braindeath, better just delete the file and start over
 */

PRAGMA(message "Delete this file when no users left")
DIAGNOSTIC(ignored "-Wdeprecated-declarations")

void *_FREEREF;

#ifdef DEBUG
	// #define DEBUG_REFS
#endif

#ifdef DEBUG_REFS
	#define REFLOG(...) log_debug(__VA_ARGS__);
#else
	#define REFLOG(...)
#endif

int add_ref(void *ptr) {
	int i, firstfree = -1;

	for(i = 0; i < global.refs.count; i++) {
		if(global.refs.ptrs[i].ptr == ptr) {
			global.refs.ptrs[i].refs++;
			REFLOG("increased refcount for %p (ref %i): %i", ptr, i, global.refs.ptrs[i].refs);
			return i;
		} else if(firstfree < 0 && global.refs.ptrs[i].ptr == FREEREF) {
			firstfree = i;
		}
	}

	if(firstfree >= 0) {
		global.refs.ptrs[firstfree].ptr = ptr;
		global.refs.ptrs[firstfree].refs = 1;
		REFLOG("found free ref for %p: %i", ptr, firstfree);
		return firstfree;
	}

	global.refs.ptrs = realloc(global.refs.ptrs, (++global.refs.count)*sizeof(Reference));
	global.refs.ptrs[global.refs.count - 1].ptr = ptr;
	global.refs.ptrs[global.refs.count - 1].refs = 1;
	REFLOG("new ref for %p: %i", ptr, global.refs.count - 1);

	return global.refs.count - 1;
}

void del_ref(void *ptr) {
	int i;

	for(i = 0; i < global.refs.count; i++)
		if(global.refs.ptrs[i].ptr == ptr)
			global.refs.ptrs[i].ptr = NULL;
}

void free_ref(int i) {
	if(i < 0)
		return;

	global.refs.ptrs[i].refs--;
	REFLOG("decreased refcount for %p (ref %i): %i", global.refs.ptrs[i].ptr, i, global.refs.ptrs[i].refs);

	if(global.refs.ptrs[i].refs <= 0) {
		global.refs.ptrs[i].ptr = FREEREF;
		global.refs.ptrs[i].refs = 0;
		REFLOG("ref %i is now free", i);
	}
}

void free_all_refs(void) {
	int inuse = 0;
	int inuse_unique = 0;

	for(int i = 0; i < global.refs.count; i++) {
		if(global.refs.ptrs[i].refs) {
			inuse += global.refs.ptrs[i].refs;
			inuse_unique += 1;
		}
	}

	if(inuse) {
		log_warn("%i refs were still in use (%i unique, %i total allocated)", inuse, inuse_unique, global.refs.count);
	}

	free(global.refs.ptrs);
	memset(&global.refs, 0, sizeof(RefArray));
}
