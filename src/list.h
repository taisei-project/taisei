/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2017, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2017, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once

typedef struct List {
    struct List *next;
    struct List *prev;
} List;

typedef struct ListContainer {
    struct ListContainer *next;
    struct ListContainer *prev;
    void *data;
} ListContainer;

typedef void* (*ListForeachCallback)(List **head, List *elem, void *arg);
typedef int (*ListPriorityFunc)(List *elem);
typedef List* (*ListInsertionRule)(List **dest, List *elem);

List* list_insert(List **dest, List *elem);
List* list_push(List **dest, List *elem);
List* list_append(List **dest, List *elem);
List* list_insert_at_priority(List **dest, List *elem, int prio, ListPriorityFunc prio_func) __attribute__((hot));
List* list_pop(List **dest);
List* list_unlink(List **dest, List *elem);
void* list_foreach(List **dest, ListForeachCallback callback, void *arg);
void* list_callback_free_element(List **dest, List *elem, void *arg);
void list_free_all(List **dest);
ListContainer* list_wrap_container(void *data);

// type-generic macros

#ifndef LIST_NO_MACROS

#define LIST_CAST(expr,ptrlevel) (_Generic((expr), \
    ListContainer ptrlevel: (List ptrlevel)(expr), \
    void ptrlevel: (List ptrlevel)(expr), \
    List ptrlevel: (expr) \
))

#define list_insert(dest,elem) list_insert(LIST_CAST(dest, **), LIST_CAST(elem, *))
#define list_push(dest,elem) list_push(LIST_CAST(dest, **), LIST_CAST(elem, *))
#define list_append(dest,elem) list_append(LIST_CAST(dest, **), LIST_CAST(elem, *))
#define list_insert_at_priority(dest,elem,prio,prio_func) list_insert_at_priority(LIST_CAST(dest, **), LIST_CAST(elem, *), prio, prio_func)
#define list_pop(dest) list_pop(LIST_CAST(dest, **))
#define list_unlink(dest,elem) list_unlink(LIST_CAST(dest, **), LIST_CAST(elem, *))
#define list_foreach(dest,callback,arg) list_foreach(LIST_CAST(dest, **), callback, arg)
#define list_free_all(dest) list_free_all(LIST_CAST(dest, **))

#endif // LIST_NO_MACROS
