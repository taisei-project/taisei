/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information. 
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#include "list.h"

#include <stdlib.h>
#include <stdio.h>

typedef struct {
	void *next;
	void *prev;
} List;

void *create_element(void **dest, int size) {
	void *last = *dest;
	void *e = malloc(size);
	
	if(last != NULL) {
		while(((List *)last)->next != NULL)
			last = ((List *)last)->next;
		((List *)last)->next = e;
	} else {
		*dest = e;
	}
	
	((List *)e)->prev = last;
	((List *)e)->next = NULL;
	
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
