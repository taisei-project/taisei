/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (C) 2011, Lukas Weber <laochailan@web.de>
 */

#ifndef LIST_H
#define LIST_H

/* I got annoyed of the code doubling caused by simple linked lists,
 * so i do some void-magic here to save the lines.
 */

void *create_element(void **dest, int size);
void delete_element(void **dest, void *e);
void delete_all_elements(void **dest, void (callback)(void **, void *));
void delete_all_elements_witharg(void **dest, void (callback)(void **, void *, void *), void *arg);

typedef struct ListContainer {
    struct ListContainer *next;
    struct ListContainer *prev;
    void *data;
} ListContainer;

ListContainer* create_container(ListContainer **dest);

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
#define REF(p) (global.refs.ptrs[static_cast<int>(creal(p))].ptr)
int add_ref(void *ptr);
void del_ref(void *ptr);
void free_ref(_Complex double i);
void free_all_refs(void);
#endif
