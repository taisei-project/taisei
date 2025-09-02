/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "list.h"

#undef list_insert
List* list_insert(List **dest, List *elem) {
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

#undef alist_insert
List* alist_insert(ListAnchor *list, List *ref, List *elem) {
	elem->prev = ref;

	if(ref != NULL) {
		elem->next = ref->next;

		if(elem->next != NULL) {
			elem->next->prev = elem;
		}

		if(ref == list->last) {
			list->last = elem;
		}

		if(list->first == NULL) {
			// list->first = elem;
		}

		ref->next = elem;
	} else {
		assert(list->first == NULL);
		assert(list->last == NULL);
		elem->next = elem->prev = NULL;
		list->first = list->last = elem;
	}

	return elem;
}

#undef list_push
List* list_push(List **dest, List *elem) {
	if(*dest) {
		(*dest)->prev = elem;
	}

	elem->next = *dest;
	elem->prev = NULL;

	*dest = elem;
	return elem;
}

#undef alist_push
List* alist_push(ListAnchor *list, List *elem) {
	elem->next = list->first;
	elem->prev = NULL;

	if(list->last == NULL) {
		assert(list->first == NULL);
		list->last = elem;
	} else {
		assert(list->first != NULL);
		list->first->prev = elem;
	}

	list->first = elem;
	return elem;
}

#undef list_append
List* list_append(List **dest, List *elem) {
	if(*dest == NULL) {
		return list_insert(dest, elem);
	}

	List *end = *dest;
	for(List *e = (*dest)->next; e; e = e->next) {
		end = e;
	}

	return list_insert(&end, elem);
}

#undef alist_append
List* alist_append(ListAnchor *list, List *elem) {
	elem->next = NULL;
	elem->prev = list->last;

	if(list->last == NULL) {
		assert(list->first == NULL);
		list->first = elem;
	} else {
		assert(list->first != NULL);
		list->last->next = elem;
	}

	list->last = elem;
	return elem;
}

attr_hot
static List* list_insert_at_priority(List **list_head, List *elem, int prio, ListPriorityFunc prio_func, bool head) {
	if(!*list_head) {
		elem->prev = elem->next = NULL;
		*list_head = elem;
		return elem;
	}

	List *dest = *list_head;
	int dest_prio = prio_func(dest);
	int candidate_prio = dest_prio;

	if(head) {
		for(List *e = dest->next; e && (candidate_prio = prio_func(e)) <  prio; e = e->next) {
			dest = e;
			dest_prio = candidate_prio;
		}
	} else {
		for(List *e = dest->next; e && (candidate_prio = prio_func(e)) <= prio; e = e->next) {
			dest = e;
			dest_prio = candidate_prio;
		}
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

#undef list_insert_at_priority_head
List* list_insert_at_priority_head(List **dest, List *elem, int prio, ListPriorityFunc prio_func) {
	return list_insert_at_priority(dest, elem, prio, prio_func, true);
}

#undef alist_insert_at_priority_head
List* alist_insert_at_priority_head(ListAnchor *list, List *elem, int prio, ListPriorityFunc prio_func) {
	if(list->first == NULL) {
		assert(list->last == NULL);
		elem->prev = elem->next = NULL;
		list->first = list->last = elem;
		return elem;
	}

	List *dest = list->first;
	int dest_prio = prio_func(dest);
	int candidate_prio = dest_prio;

	for(List *e = dest->next; e && (candidate_prio = prio_func(e)) < prio; e = e->next) {
		dest = e;
		dest_prio = candidate_prio;
	}

	if(dest == list->first && dest_prio > prio) {
		elem->next = dest;
		elem->prev = dest->prev;

		if(elem->prev) {
			elem->prev->next = elem;
		}

		dest->prev = elem;
		list->first = elem;
	} else {
		elem->prev = dest;
		elem->next = dest->next;

		if(dest->next) {
			dest->next->prev = elem;
		} else {
			assert(dest == list->last);
			list->last = elem;
		}

		dest->next = elem;
	}

	return elem;
}

#undef list_insert_at_priority_tail
List* list_insert_at_priority_tail(List **dest, List *elem, int prio, ListPriorityFunc prio_func) {
	return list_insert_at_priority(dest, elem, prio, prio_func, false);
}

#undef alist_insert_at_priority_tail
List* alist_insert_at_priority_tail(ListAnchor *list, List *elem, int prio, ListPriorityFunc prio_func) {
	if(list->last == NULL) {
		assert(list->first == NULL);
	}

	if(list->first == NULL) {
		assert(list->last == NULL);
		elem->prev = elem->next = NULL;
		list->first = list->last = elem;
		return elem;
	}

	List *dest = list->last;
	int dest_prio = prio_func(dest);

	for(List *e = dest->prev; e && dest_prio > prio; e = e->prev) {
		dest = e;
		dest_prio = prio_func(e);
	}

	if(dest == list->first && dest_prio > prio) {
		elem->next = dest;
		elem->prev = dest->prev;

		if(elem->prev) {
			elem->prev->next = elem;
		}

		dest->prev = elem;
		list->first = elem;
	} else {
		elem->prev = dest;
		elem->next = dest->next;

		if(dest->next) {
			dest->next->prev = elem;
		} else {
			assert(dest == list->last);
			list->last = elem;
		}

		dest->next = elem;
	}

	return elem;
}

#undef list_unlink
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

#undef alist_unlink
List* alist_unlink(ListAnchor *list, List *elem) {
	if(list->last == elem) {
		list->last = list->last->prev;
	}

	return list_unlink(&list->first, elem);
}

#undef list_pop
List* list_pop(List **dest) {
	if(*dest == NULL) {
		return NULL;
	}

	return list_unlink(dest, *dest);
}

#undef alist_pop
List* alist_pop(ListAnchor *list) {
	if(list->first == NULL) {
		return NULL;
	}

	return alist_unlink(list, list->first);
}

#undef alist_merge_tail
void alist_merge_tail(ListAnchor *dest, ListAnchor *src) {
	if(src->first) {
		src->first->prev = dest->last;

		if(dest->last) {
			assume(dest->first != NULL);
			dest->last->next = src->first;
		} else {
			assume(dest->first == NULL);
			dest->first = src->first;
		}

		dest->last = src->last;
		src->first = NULL;
		src->last = NULL;
	}
}

#undef list_foreach
void* list_foreach(List **dest, ListForeachCallback callback, void *arg) {
	void *ret = NULL;

	for(List *e = *dest, *next; e; e = next) {
		next = e->next;

		if((ret = callback(dest, e, arg)) != NULL) {
			return ret;
		}
	}

	return ret;
}

#undef alist_foreach
void* alist_foreach(ListAnchor *list, ListAnchorForeachCallback callback, void *arg) {
	void *ret = NULL;

	for(List *e = list->first, *next; e; e = next) {
		next = e->next;

		if((ret = callback(list, e, arg)) != NULL) {
			return ret;
		}
	}

	return ret;
}

void* list_callback_free_element(List **dest, List *elem, void *arg) {
	list_unlink(dest, elem);
	mem_free(elem);
	return NULL;
}

void* alist_callback_free_element(ListAnchor *list, List *elem, void *arg) {
	alist_unlink(list, elem);
	mem_free(elem);
	return NULL;
}

#undef list_free_all
void list_free_all(List **dest) {
	list_foreach(dest, list_callback_free_element, NULL);
}

#undef alist_free_all
void alist_free_all(ListAnchor *list) {
	alist_foreach(list, alist_callback_free_element, NULL);
}

ListContainer* list_wrap_container(void *data) {
	return ALLOC(ListContainer, { .data = data });
}
