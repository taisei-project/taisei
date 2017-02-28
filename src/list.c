/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "list.h"

#include <stdlib.h>
#include <stdio.h>
#include "global.h"

typedef struct {
	void *next;
	void *prev;
} List;

void *_FREEREF;

void *create_element(void **dest, int size) {
	List *e = malloc(size);
	List **d = (List **)dest;

	e->next = NULL;
	e->prev = *d;

	if(*d != NULL) {
		e->next = (*d)->next;
		if((*d)->next)
			((List *)(*d)->next)->prev = e;

		(*d)->next = e;
	} else {
		*d = e;
	}

	return e;
}

void delete_element(void **dest, void *e) {
	if(((List *)e)->prev != NULL)
		((List *)((List *)e)->prev)->next = ((List *)e)->next;
	if(((List *)e)->next != NULL)
		((List *)((List *)e)->next)->prev = ((List *)e)->prev;
	if(*dest == e)
		*dest = ((List *)e)->next;

	free(e);
}

void delete_all_elements(void **dest, void (callback)(void **, void *)) {
	void *e = *dest;
	void *tmp;

	while(e != 0) {
		tmp = e;
		e = ((List *)e)->next;
		callback(dest, tmp);
	}

	*dest = NULL;
}

#ifdef DEBUG
	// #define DEBUG_REFS
#endif

#ifdef DEBUG_REFS
	#define REFLOG(...) fprintf(stderr, __VA_ARGS__);
#else
	#define REFLOG(...)
#endif

int add_ref(void *ptr) {
	int i, firstfree = -1;

	for(i = 0; i < global.refs.count; i++) {
		if(global.refs.ptrs[i].ptr == ptr) {
			global.refs.ptrs[i].refs++;
			REFLOG("increased refcount for %p (ref %i): %i\n", ptr, i, global.refs.ptrs[i].refs);
			return i;
		} else if(firstfree < 0 && global.refs.ptrs[i].ptr == FREEREF) {
			firstfree = i;
		}
	}

	if(firstfree >= 0) {
		global.refs.ptrs[firstfree].ptr = ptr;
		global.refs.ptrs[firstfree].refs = 1;
		REFLOG("found free ref for %p: %i\n", ptr, firstfree);
		return firstfree;
	}

	global.refs.ptrs = realloc(global.refs.ptrs, (++global.refs.count)*sizeof(Reference));
	global.refs.ptrs[global.refs.count - 1].ptr = ptr;
	global.refs.ptrs[global.refs.count - 1].refs = 1;
	REFLOG("new ref for %p: %i\n", ptr, global.refs.count - 1);

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
	REFLOG("decreased refcount for %p (ref %i): %i\n", global.refs.ptrs[i].ptr, i, global.refs.ptrs[i].refs);

	if(global.refs.ptrs[i].refs <= 0) {
		global.refs.ptrs[i].ptr = FREEREF;
		global.refs.ptrs[i].refs = 0;
		REFLOG("ref %i is now free\n", i);
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
		warnx("free_all_refs(): %i refs were still in use (%i unique, %i total allocated)", inuse, inuse_unique, global.refs.count);
	}

	free(global.refs.ptrs);
	memset(&global.refs, 0, sizeof(RefArray));
}
