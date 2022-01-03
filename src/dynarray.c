/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "dynarray.h"
#include "util.h"

// #define DYNARRAY_DEBUG

#ifdef DYNARRAY_DEBUG
	#undef DYNARRAY_DEBUG
	#define DYNARRAY_DEBUG(darr, fmt, ...) log_debug("[%p] " fmt, (void*)(darr), __VA_ARGS__)
#else
	#define DYNARRAY_DEBUG(...) ((void)0)
#endif

void _dynarray_free_data(dynarray_size_t sizeof_element, DynamicArray *darr) {
	free(darr->data);
	if(darr->capacity) {
		DYNARRAY_DEBUG(darr, "%u/%u", darr->num_elements, darr->capacity);
	}
	memset(darr, 0, sizeof(*darr));
}

INLINE void _dynarray_resize(dynarray_size_t sizeof_element, DynamicArray *darr, dynarray_size_t capacity) {
	assert(sizeof_element > 0);
	assert(capacity > 0);
	DYNARRAY_DEBUG(darr, "capacity change: %u --> %u", darr->capacity, capacity);
	darr->capacity = capacity;
	darr->data = realloc(darr->data, sizeof_element * capacity);
}

void _dynarray_ensure_capacity(dynarray_size_t sizeof_element, DynamicArray *darr, dynarray_size_t capacity) {
	if(darr->capacity < capacity) {
		_dynarray_resize(sizeof_element, darr, capacity);
	}
}

dynarray_size_t _dynarray_prepare_append_with_min_capacity(dynarray_size_t sizeof_element, DynamicArray *darr, dynarray_size_t min_capacity) {
	dynarray_size_t num_elements = darr->num_elements;
	dynarray_size_t capacity = darr->capacity;

	assume(num_elements >= 0);
	assume(num_elements <= capacity);
	assume(min_capacity >= 2);

	if(capacity < min_capacity) {
		_dynarray_resize(sizeof_element, darr, min_capacity);
	} else if(num_elements == capacity) {
		capacity += capacity >> 1;
		_dynarray_resize(sizeof_element, darr, capacity);
	}

	++darr->num_elements;
	memset((char*)darr->data + num_elements * sizeof_element, 0, sizeof_element);

	DYNARRAY_DEBUG(darr, "elements: %u/%u", darr->num_elements, darr->capacity);
	return num_elements;  // insertion index
}

void _dynarray_compact(dynarray_size_t sizeof_element, DynamicArray *darr) {
	if(darr->capacity > darr->num_elements) {
		dynarray_size_t newsize = darr->num_elements;

		if(UNLIKELY(newsize < 1)) {
			newsize = 1;

			if(UNLIKELY(darr->capacity) == newsize) {
				return;
			}
		}

		_dynarray_resize(sizeof_element, darr, newsize);
	}
}

void _dynarray_set_elements(dynarray_size_t sizeof_element, DynamicArray *darr, dynarray_size_t num_elements, void *elements) {
	_dynarray_ensure_capacity(sizeof_element, darr, num_elements);
	memcpy(darr->data, elements, num_elements * sizeof_element);
	darr->num_elements = num_elements;
}

void _dynarray_filter(dynarray_size_t sizeof_element, DynamicArray *darr, dynarray_filter_predicate_t predicate, void *userdata) {
	char *p = darr->data;
	char *end = p + sizeof_element * darr->num_elements;
	dynarray_size_t shift = 0;

	while(p < end) {
		if(!predicate(p, userdata)) {
			shift += sizeof_element;
			--darr->num_elements;
		} else if(shift) {
			memmove(p - shift, p, sizeof_element);
		}

		p += sizeof_element;
	}
}
