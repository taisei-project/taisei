/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include "list.h"

#include <stdlib.h>
#include <stdio.h>
#include "global.h"

void *_FREEREF;

void *create_element(void **dest, size_t size) {
	assert(dest != NULL);
	assert(size > 0);

	List *e = calloc(1, size);
	List **d = (List **)dest;

	e->prev = *d;

	if(*d != NULL) {
		e->next = (*d)->next;
		if((*d)->next)
			(*d)->next->prev = e;

		(*d)->next = e;
	} else {
		*d = e;
	}

	return e;
}

void* create_element_at_end(void **dest, size_t size) {
	assert(dest != NULL);

	if(*dest == NULL) {
		return create_element((void **)dest,size);
	}

	List *end = NULL;
	for(List *e = *dest; e; e = e->next) {
		end = e;
	}

	return create_element((void **)&end,size);
}

void* create_element_at_priority(void **list_head, size_t size, int prio, int (*prio_func)(void*)) {
	assert(list_head != NULL);
	assert(size > 0);
	assert(prio_func != NULL);

	List *elem = calloc(1, size);

	if(!*list_head) {
		*list_head = elem;
		log_debug("PRIO: %i", prio);
		return elem;
	}

	List *dest = *list_head;

	for(List *e = dest; e && prio_func(e) <= prio; e = e->next) {
		dest = e;
	}

	if(dest == *list_head && prio_func(dest) > prio) {
		elem->next = dest;
		elem->prev = dest->prev;

		if(elem->prev) {
			elem->prev->next = elem;
		}

		dest->prev = elem;
		*list_head = elem;
	} else {
		elem->prev = dest;
		elem->next = dest->next;

		if(dest->next) {
			dest->next->prev = elem;
		}

		dest->next = elem;
	}

	return elem;
}

ListContainer* create_container(ListContainer **dest) {
	return create_element((void**)dest, sizeof(ListContainer));
}

ListContainer* create_container_at_priority(ListContainer **list_head, int prio, int (*prio_func)(void*)) {
	return create_element_at_priority((void**)list_head, sizeof(ListContainer), prio, prio_func);
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

void delete_all_elements_witharg(void **dest, void (callback)(void **, void *, void *), void *arg) {
	void *e = *dest;
	void *tmp;

	while(e != 0) {
		tmp = e;
		e = ((List *)e)->next;
		callback(dest, tmp, arg);
	}

	*dest = NULL;
}

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
