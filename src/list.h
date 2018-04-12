/*
 * This software is licensed under the terms of the MIT-License
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2018, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2018, Andrei Alexeyev <akari@alienslab.net>.
 */

#pragma once
#include "taisei.h"

typedef struct ListInterface ListInterface;
typedef struct List List;
typedef struct ListContainer ListContainer;

#define LIST_INTERFACE_BASE(typename) struct { \
	typename *next; \
	typename *prev; \
}

#define LIST_INTERFACE(typename) union { \
	ListInterface list_interface; \
	LIST_INTERFACE_BASE(typename); \
}

struct ListInterface {
	LIST_INTERFACE_BASE(ListInterface);
};

struct List {
	LIST_INTERFACE(List);
};

struct ListContainer {
	LIST_INTERFACE(ListContainer);
	void *data;
};

typedef void* (*ListForeachCallback)(List **head, List *elem, void *arg);
typedef int (*ListPriorityFunc)(List *elem);
typedef List* (*ListInsertionRule)(List **dest, List *elem);

List* list_insert(List **dest, List *elem) attr_nonnull(1, 2);
List* list_push(List **dest, List *elem) attr_nonnull(1, 2);
List* list_append(List **dest, List *elem) attr_nonnull(1, 2);
List* list_insert_at_priority_head(List **dest, List *elem, int prio, ListPriorityFunc prio_func) attr_hot attr_nonnull(1, 2, 4);
List* list_insert_at_priority_tail(List **dest, List *elem, int prio, ListPriorityFunc prio_func) attr_hot attr_nonnull(1, 2, 4);
List* list_pop(List **dest) attr_nonnull(1);
List* list_unlink(List **dest, List *elem) attr_nonnull(1, 2);
void* list_foreach(List **dest, ListForeachCallback callback, void *arg) attr_nonnull(1, 2);
void* list_callback_free_element(List **dest, List *elem, void *arg);
void list_free_all(List **dest) attr_nonnull(1);
ListContainer* list_wrap_container(void *data) attr_nodiscard;

// type-generic macros

#ifndef LIST_NO_MACROS

#ifdef USE_GNU_EXTENSIONS
	// thorough safeguard
	#define LIST_CAST(expr,ptrlevel) (__extension__({ \
		static_assert(__builtin_types_compatible_p(ListInterface, __typeof__((ptrlevel (expr)).list_interface)), \
			"struct must implement ListInterface (use the LIST_INTERFACE macro)"); \
		static_assert(__builtin_offsetof(__typeof__(ptrlevel (expr)), list_interface) == 0, \
			"list_interface must be the first member in struct"); \
		(List ptrlevel)(expr); \
	}))
	#define LIST_CAST_RETURN(expr) (__typeof__(expr))
#else
	// basic safeguard
	#define LIST_CAST(expr,ptrlevel) ((void)sizeof((ptrlevel (expr)).list_interface), (List ptrlevel)(expr))
	// don't even think about adding a void* cast here
	#define LIST_CAST_RETURN(expr)
#endif

#define list_insert(dest,elem) \
	(LIST_CAST_RETURN(elem) list_insert(LIST_CAST(dest, **), LIST_CAST(elem, *)))

#define list_push(dest,elem) \
	(LIST_CAST_RETURN(elem) list_push(LIST_CAST(dest, **), LIST_CAST(elem, *)))

#define list_append(dest,elem) \
	(LIST_CAST_RETURN(elem) list_append(LIST_CAST(dest, **), LIST_CAST(elem, *)))

#define list_insert_at_priority_head(dest,elem,prio,prio_func) \
	(LIST_CAST_RETURN(elem) list_insert_at_priority_head(LIST_CAST(dest, **), LIST_CAST(elem, *), prio, prio_func))

#define list_insert_at_priority_tail(dest,elem,prio,prio_func) \
	(LIST_CAST_RETURN(elem) list_insert_at_priority_tail(LIST_CAST(dest, **), LIST_CAST(elem, *), prio, prio_func))

#define list_pop(dest) \
	(LIST_CAST_RETURN(*(dest)) list_pop(LIST_CAST(dest, **)))

#define list_unlink(dest,elem) \
	(LIST_CAST_RETURN(elem) list_unlink(LIST_CAST(dest, **), LIST_CAST(elem, *)))

#define list_foreach(dest,callback,arg) \
	list_foreach(LIST_CAST(dest, **), callback, arg)

#define list_free_all(dest) \
	list_free_all(LIST_CAST(dest, **))

#endif // LIST_NO_MACROS
