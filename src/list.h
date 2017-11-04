/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once

/* I got annoyed of the code doubling caused by simple linked lists,
 * so i do some void-magic here to save the lines.
 */

#include <stdlib.h>

void* create_element(void **dest, size_t size);
void* create_element_at_end(void **dest, size_t size);
void* create_element_at_priority(void **list_head, size_t size, int prio, int (*prio_func)(void*));
void delete_element(void **dest, void *e);
void delete_all_elements(void **dest, void (callback)(void **, void *));
void delete_all_elements_witharg(void **dest, void (callback)(void **, void *, void *), void *arg);

typedef struct List {
    struct List *next;
    struct List *prev;
} List;

typedef struct ListContainer {
    struct ListContainer *next;
    struct ListContainer *prev;
    void *data;
} ListContainer;

ListContainer* create_container(ListContainer **dest);
ListContainer* create_container_at_priority(ListContainer **list_head, int prio, int (*prio_func)(void*));

typedef struct {
	void *ptr;
	int refs;
} Reference;

typedef struct {
	Reference *ptrs;
	int count;
} RefArray;

extern void *_FREEREF;
#define FREEREF &_FREEREF
#define REF(p) (global.refs.ptrs[(int)(p)].ptr)
int add_ref(void *ptr);
void del_ref(void *ptr);
void free_ref(int i);
void free_all_refs(void);
