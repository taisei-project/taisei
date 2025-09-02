/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#pragma once
#include "taisei.h"

#if TAISEI_BUILDCONF_MALLOC_ALIGNMENT < 16
	#define LIST_ALIGN alignas(TAISEI_BUILDCONF_MALLOC_ALIGNMENT)
#else
	#define LIST_ALIGN alignas(16)
#endif

typedef struct ListInterface ListInterface;
typedef struct List List;
typedef struct ListAnchorInterface ListAnchorInterface;
typedef struct ListAnchor ListAnchor;
typedef struct ListContainer ListContainer;

#define LIST_INTERFACE_BASE(typename) struct { \
	typename *next; \
	typename *prev; \
}

#define LIST_INTERFACE(typename) union { \
	LIST_ALIGN ListInterface list_interface; \
	LIST_INTERFACE_BASE(typename); \
}

struct ListInterface {
	LIST_INTERFACE_BASE(ListInterface);
};

struct List {
	LIST_INTERFACE(List);
};

#define LIST_ANCHOR_INTERFACE_BASE(typename) struct { \
	typename *first; \
	typename *last; \
}

#define LIST_ANCHOR_INTERFACE(typename) union { \
	ListAnchorInterface list_anchor_interface; \
	LIST_ANCHOR_INTERFACE_BASE(typename); \
}

#define LIST_ANCHOR(typename) struct { \
	LIST_ANCHOR_INTERFACE(typename); \
}

struct ListAnchorInterface {
	LIST_ANCHOR_INTERFACE_BASE(ListInterface);
};

struct ListAnchor {
	LIST_ANCHOR_INTERFACE(List);
};

struct ListContainer {
	LIST_INTERFACE(ListContainer);
	void *data;
};

typedef void* (*ListForeachCallback)(List **head, List *elem, void *arg);
typedef void* (*ListAnchorForeachCallback)(ListAnchor *list, List *elem, void *arg);
typedef List* (*ListInsertionRule)(List **dest, List *elem);
typedef List* (*ListAnchorInsertionRule)(ListAnchor *dest, List *elem);
typedef int (*ListPriorityFunc)(List *elem);

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

List* alist_insert(ListAnchor *list, List *ref, List *elem) attr_nonnull(1, 3);
List* alist_push(ListAnchor *list, List *elem) attr_nonnull(1, 2);
List* alist_append(ListAnchor *list, List *elem) attr_nonnull(1, 2);
List* alist_insert_at_priority_head(ListAnchor *list, List *elem, int prio, ListPriorityFunc prio_func) attr_hot attr_nonnull(1, 2, 4);
List* alist_insert_at_priority_tail(ListAnchor *list, List *elem, int prio, ListPriorityFunc prio_func) attr_hot attr_nonnull(1, 2, 4);
List* alist_pop(ListAnchor *list) attr_nonnull(1);
List* alist_unlink(ListAnchor *list, List *elem) attr_nonnull(1, 2);
void alist_merge_tail(ListAnchor *dest, ListAnchor *src) attr_nonnull(1, 2);
void* alist_foreach(ListAnchor *list, ListAnchorForeachCallback callback, void *arg) attr_nonnull(1, 2);
void* alist_callback_free_element(ListAnchor *list, List *elem, void *arg);
void alist_free_all(ListAnchor *list) attr_nonnull(1);

ListContainer* list_wrap_container(void *data) attr_returns_allocated;

// type-generic macros

#define LIST_CAST(expr) ({ \
	static_assert(__builtin_types_compatible_p( \
		ListInterface, __typeof__((*(expr)).list_interface)), \
		"struct must implement ListInterface (use the LIST_INTERFACE macro)"); \
	static_assert(__builtin_offsetof(__typeof__(*(expr)), list_interface) == 0, \
		"list_interface must be the first member in struct"); \
	CASTPTR_ASSUME_ALIGNED((expr), List); \
})

#define LIST_CAST_2(expr) ({ \
	static_assert(__builtin_types_compatible_p(\
		ListInterface, __typeof__((**(expr)).list_interface)), \
		"struct must implement ListInterface (use the LIST_INTERFACE macro)"); \
	static_assert(__builtin_offsetof(__typeof__(**(expr)), list_interface) == 0, \
		"list_interface must be the first member in struct"); \
	(void)ASSUME_ALIGNED(*(expr), alignof(List)); \
	(List**)(expr); \
})

#define LIST_ANCHOR_CAST(expr) ({ \
	static_assert(__builtin_types_compatible_p(\
		ListAnchorInterface, __typeof__((*(expr)).list_anchor_interface)), \
		"struct must implement ListAnchorInterface (use the LIST_ANCHOR_INTERFACE macro)"); \
	static_assert(__builtin_offsetof(__typeof__(*(expr)), list_anchor_interface) == 0, \
		"list_anchor_interface must be the first member in struct"); \
	CASTPTR_ASSUME_ALIGNED((expr), ListAnchor); \
})

#define LIST_CAST_RETURN(e_typekey, e_return) \
	CASTPTR_ASSUME_ALIGNED((e_return), __typeof__(*(e_typekey)))

#define list_insert(dest,elem) \
	(LIST_CAST_RETURN(elem, list_insert(LIST_CAST_2(dest), LIST_CAST(elem))))

#define alist_insert(dest,ref,elem) \
	(LIST_CAST_RETURN(elem, alist_insert(LIST_ANCHOR_CAST(dest), LIST_CAST(ref), LIST_CAST(elem))))

#define list_push(dest,elem) \
	(LIST_CAST_RETURN(elem, list_push(LIST_CAST_2(dest), LIST_CAST(elem))))

#define alist_push(dest,elem) \
	(LIST_CAST_RETURN(elem, alist_push(LIST_ANCHOR_CAST(dest), LIST_CAST(elem))))

#define list_append(dest,elem) \
	(LIST_CAST_RETURN(elem, list_append(LIST_CAST_2(dest), LIST_CAST(elem))))

#define alist_append(dest,elem) \
	(LIST_CAST_RETURN(elem, alist_append(LIST_ANCHOR_CAST(dest), LIST_CAST(elem))))

#define list_insert_at_priority_head(dest,elem,prio,prio_func) \
	(LIST_CAST_RETURN(elem, list_insert_at_priority_head(LIST_CAST_2(dest), LIST_CAST(elem), prio, prio_func)))

#define alist_insert_at_priority_head(dest,elem,prio,prio_func) \
	(LIST_CAST_RETURN(elem, alist_insert_at_priority_head(LIST_ANCHOR_CAST(dest), LIST_CAST(elem), prio, prio_func)))

#define list_insert_at_priority_tail(dest,elem,prio,prio_func) \
	(LIST_CAST_RETURN(elem, list_insert_at_priority_tail(LIST_CAST_2(dest), LIST_CAST(elem), prio, prio_func)))

#define alist_insert_at_priority_tail(dest,elem,prio,prio_func) \
	(LIST_CAST_RETURN(elem, alist_insert_at_priority_tail(LIST_ANCHOR_CAST(dest), LIST_CAST(elem), prio, prio_func)))

#define list_pop(dest) \
	(LIST_CAST_RETURN(*(dest), list_pop(LIST_CAST_2(dest))))

#define alist_pop(dest) \
	(LIST_CAST_RETURN((dest)->first, alist_pop(LIST_ANCHOR_CAST(dest))))

#define list_unlink(dest,elem) \
	(LIST_CAST_RETURN(elem, list_unlink(LIST_CAST_2(dest), LIST_CAST(elem))))

#define alist_unlink(dest,elem) \
	(LIST_CAST_RETURN(elem, alist_unlink(LIST_ANCHOR_CAST(dest), LIST_CAST(elem))))

#define alist_merge_tail(dest,src) \
	alist_merge_tail(LIST_ANCHOR_CAST(dest), LIST_ANCHOR_CAST(src))

#define list_foreach(dest,callback,arg) \
	list_foreach(LIST_CAST_2(dest), callback, arg)

#define alist_foreach(dest,callback,arg) \
	alist_foreach(LIST_ANCHOR_CAST(dest), callback, arg)

#define list_free_all(dest) \
	list_free_all(LIST_CAST_2(dest))

#define alist_free_all(dest) \
	alist_free_all(LIST_ANCHOR_CAST(dest))
