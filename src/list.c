/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#include <stdlib.h>
#include <stdio.h>

#define LIST_NO_MACROS

#include "list.h"
#include "global.h"

List* list_insert(List **dest, List *elem) {
	assert(dest != NULL);
	assert(elem != NULL);

	elem->prev = *dest;

	if(*dest != NULL) {
		elem->next = (*dest)->next;

		if((*dest)->next) {
			(*dest)->next->prev = elem;
		}

		(*dest)->next = elem;
	} else {
		elem->next = NULL;
		*dest = elem;
	}

	return elem;
}

List* list_push(List **dest, List *elem) {
	assert(dest != NULL);
	assert(elem != NULL);

	if(*dest) {
		(*dest)->prev = elem;
	}

	elem->next = *dest;
	elem->prev = NULL;

	*dest = elem;
	return elem;
}

List* list_append(List **dest, List *elem) {
	assert(dest != NULL);
	assert(elem != NULL);

	if(*dest == NULL) {
		return list_insert(dest, elem);
	}

	List *end = *dest;
	for(List *e = (*dest)->next; e; e = e->next) {
		end = e;
	}

	return list_insert(&end, elem);
}

List* list_insert_at_priority(List **list_head, List *elem, int prio, ListPriorityFunc prio_func) {
	assert(list_head != NULL);
	assert(elem != NULL);
	assert(prio_func != NULL);

	if(!*list_head) {
		elem->prev = elem->next = NULL;
		*list_head = elem;
		return elem;
	}

	List *dest = *list_head;
	int dest_prio = prio_func(dest);
	int candidate_prio = dest_prio;

	for(List *e = dest->next; e && (candidate_prio = prio_func(e)) <= prio; e = e->next) {
		dest = e;
		dest_prio = candidate_prio;
	}

	if(dest == *list_head && dest_prio > prio) {
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

List* list_unlink(List **dest, List *elem) {
	if(elem->prev != NULL) {
		elem->prev->next = elem->next;
	}

	if(elem->next != NULL)  {
		elem->next->prev = elem->prev;
	}

	if(*dest == elem) {
		*dest = elem->next;
	}

	return elem;
}

List* list_pop(List **dest) {
	if(*dest == NULL) {
		return NULL;
	}

	return list_unlink(dest, *dest);
}

void* list_foreach(List **dest, ListForeachCallback callback, void *arg) {
	List *e = *dest;

	while(e != 0) {
		void *ret;
		List *tmp = e;
		e = e->next;

		if((ret = callback(dest, tmp, arg)) != NULL) {
			return ret;
		}
	}

	return NULL;
}

void* list_callback_free_element(List **dest, List *elem, void *arg) {
	list_unlink(dest, elem);
	free(elem);
	return NULL;
}

void list_free_all(List **dest) {
	list_foreach(dest, list_callback_free_element, NULL);
}

ListContainer* list_wrap_container(void *data) {
	ListContainer *c = calloc(1, sizeof(ListContainer));
	c->data = data;
	return c;
}
