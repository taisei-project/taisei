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

int add_ref(void *ptr) {
	int i;

	for(i = 0; i < global.refs.count; i++) {
		if(global.refs.ptrs[i].ptr == ptr) {
			global.refs.ptrs[i].refs++;
			return i;
		}
	}

	for(i = 0; i < global.refs.count; i++) {
		if(global.refs.ptrs[i].ptr == FREEREF) {
			global.refs.ptrs[i].ptr = ptr;
			global.refs.ptrs[i].refs = 1;
			return i;
		}
	}

	global.refs.ptrs = realloc(global.refs.ptrs, (++global.refs.count)*sizeof(Reference));
	global.refs.ptrs[global.refs.count - 1].ptr = ptr;
	global.refs.ptrs[global.refs.count - 1].refs = 1;

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
	if(global.refs.ptrs[i].refs <= 0) {
		global.refs.ptrs[i].ptr = FREEREF;
		global.refs.ptrs[i].refs = 0;
	}
}