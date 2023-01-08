/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#pragma once
#include "taisei.h"

#include "util/macrohax.h"

// TODO document this mess

// NOTE/FIXME: this should be signed to avoid some subtle problems when
// interacting with other signed values. In the future we should enable implicit
// signedness conversion warnings and deal with the issue globally.
typedef int32_t dynarray_size_t;

#define DYNAMIC_ARRAY_BASE(elem_type) struct { \
	elem_type *data; \
	dynarray_size_t num_elements; \
	dynarray_size_t capacity; \
} \

typedef DYNAMIC_ARRAY_BASE(void) DynamicArray;

#define DYNAMIC_ARRAY(elem_type) union { \
	DYNAMIC_ARRAY_BASE(elem_type); \
	DynamicArray dyn_array; \
}

#define DYNARRAY_ASSERT_VALID(darr) do { \
	static_assert( \
		__builtin_types_compatible_p(DynamicArray, __typeof__((darr)->dyn_array)), \
		"x->dyn_array must be of DynamicArray type"); \
	static_assert( \
		__builtin_offsetof(__typeof__(*(darr)), dyn_array) == 0, \
		"x->dyn_array must be the first member in struct"); \
} while(0)

#define DYNARRAY_CAST_TO_BASE(darr) ({ \
	DYNARRAY_ASSERT_VALID(darr); \
	&NOT_NULL(darr)->dyn_array; \
})

#define DYNARRAY_CHECK_ELEMENT_TYPE(darr, elem) do { \
	static_assert( \
		__builtin_types_compatible_p(__typeof__(elem), __typeof__(*(darr)->data)), \
		"Dynamic array element type mismatch" \
	); \
} while(0)

#define DYNARRAY_ELEM_SIZE(darr) sizeof(*(darr)->data)

#define _dynarray_dispatch_func(func, darr, ...) \
	_dynarray_##func(DYNARRAY_ELEM_SIZE(darr), DYNARRAY_CAST_TO_BASE(darr), __VA_ARGS__)

#define _dynarray_dispatch_func_noargs(func, darr) \
	_dynarray_##func(DYNARRAY_ELEM_SIZE(darr), DYNARRAY_CAST_TO_BASE(darr))

void _dynarray_free_data(dynarray_size_t sizeof_element, DynamicArray *darr) attr_nonnull_all;
#define dynarray_free_data(darr) \
	_dynarray_dispatch_func_noargs(free_data, darr)

void _dynarray_ensure_capacity(dynarray_size_t sizeof_element, DynamicArray *darr, dynarray_size_t capacity) attr_nonnull_all;
#define dynarray_ensure_capacity(darr, capacity) \
	_dynarray_dispatch_func(ensure_capacity, darr, capacity)

dynarray_size_t _dynarray_prepare_append_with_min_capacity(dynarray_size_t sizeof_element, DynamicArray *darr, dynarray_size_t min_capacity) attr_nonnull_all;

/*
 * NOTE: unfortunately this evaluates `darr` more than once, but when compiling
 * with NDEBUG the assume() macro should trip a clang warning if there are
 * potential side-effects, and we always treat that warning as an error.
 */
#define _dynarray_append_with_min_capacity(darr, min_capacity) \
	dynarray_get_ptr(darr, _dynarray_dispatch_func(prepare_append_with_min_capacity, darr, min_capacity))

#define dynarray_append_with_min_capacity(darr, min_capacity) ({ \
	auto _darr2 = NOT_NULL(darr); \
	dynarray_get_ptr(_darr2, _dynarray_dispatch_func(prepare_append_with_min_capacity, _darr2, min_capacity)); \
})

#define dynarray_append(darr) \
	dynarray_append_with_min_capacity(darr, 2)

void _dynarray_compact(dynarray_size_t sizeof_element, DynamicArray *darr) attr_nonnull_all;
#define dynarray_compact(darr) \
	_dynarray_dispatch_func_noargs(compact, darr)

#define dynarray_qsort(darr, compare_func) do { \
	DynamicArray *_darr = DYNARRAY_CAST_TO_BASE(darr); \
	if(_darr->data) { \
		qsort(_darr->data, _darr->num_elements, DYNARRAY_ELEM_SIZE(darr), (compare_func)); \
	} \
} while(0)

#define dynarray_get_ptr(darr, idx) ({ \
	DYNARRAY_ASSERT_VALID(darr); \
	auto _darr = NOT_NULL(darr); \
	dynarray_size_t _darr_idx = (idx); \
	assume(_darr_idx >= 0); \
	assume(_darr_idx < _darr->num_elements); \
	NOT_NULL((darr)->data + _darr_idx); \
})

#define dynarray_get(darr, idx)       (*dynarray_get_ptr(darr, idx))
#define dynarray_set(darr, idx, ...) ((*dynarray_get_ptr(darr, idx)) = (__VA_ARGS__))

void _dynarray_set_elements(
	dynarray_size_t sizeof_element, DynamicArray *darr,
	dynarray_size_t num_elements, void *elements
) attr_nonnull_all;
#define dynarray_set_elements(darr, num_elements, elements) do { \
	DYNARRAY_ASSERT_VALID(darr); \
	DYNARRAY_CHECK_ELEMENT_TYPE(darr, *(elements)); \
	_dynarray_dispatch_func(set_elements, darr, num_elements, elements); \
} while(0)

typedef bool (*dynarray_filter_predicate_t)(const void *pelem, void *userdata);
void _dynarray_filter(
	dynarray_size_t sizeof_element, DynamicArray *darr,
	dynarray_filter_predicate_t predicate, void *userdata
) attr_nonnull(2, 3);
#define dynarray_filter(darr, predicate, userdata) \
	_dynarray_dispatch_func(filter, darr, predicate, userdata)

#define dynarray_indexof(darr, pelem) ({ \
	DYNARRAY_ASSERT_VALID(darr); \
	auto _darr = NOT_NULL(darr); \
	auto _darr_pelem = NOT_NULL(pelem); \
	DYNARRAY_CHECK_ELEMENT_TYPE(_darr, *(_darr_pelem)); \
	intptr_t _darr_idx = (intptr_t)(_darr_pelem - _darr->data); \
	assume(_darr_idx >= 0); \
	assume(_darr_idx < _darr->num_elements); \
	(dynarray_size_t)_darr_idx; \
})

#define _dynarray_foreach_iter MACROHAX_ADDLINENUM(_dynarray_foreach_iter)
#define _dynarray_foreach_ignored MACROHAX_ADDLINENUM(_dynarray_foreach_ignored)

#define dynarray_foreach(_darr, _cntr_var, _pelem_var, ...) do { \
	for(dynarray_size_t _dynarray_foreach_iter = 0; \
		_dynarray_foreach_iter < NOT_NULL(_darr)->num_elements; \
		++_dynarray_foreach_iter \
	) { \
		_cntr_var = _dynarray_foreach_iter; \
		_pelem_var = dynarray_get_ptr((_darr), _dynarray_foreach_iter); \
		__VA_ARGS__ \
	} \
} while(0)

#define dynarray_foreach_elem(_darr, _pelem_var, ...) \
	dynarray_foreach( \
		_darr, \
		attr_unused dynarray_size_t _dynarray_foreach_ignored, \
		_pelem_var, \
		__VA_ARGS__ \
	)

#define dynarray_foreach_idx(_darr, _cntr_var, ...) \
	dynarray_foreach(_darr, \
		_cntr_var, \
		attr_unused void *_dynarray_foreach_ignored, \
		__VA_ARGS__ \
	)
