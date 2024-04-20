/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#pragma once
#include "taisei.h"

#include "util/compat.h"
#include "util/macrohax.h"

typedef int32_t ringbuf_size_t;

#define RING_BUFFER_BASE(elem_type) struct { \
	elem_type *data; \
	ringbuf_size_t head; \
	ringbuf_size_t tail; \
	ringbuf_size_t capacity; \
	ringbuf_size_t num_elements; \
}

typedef RING_BUFFER_BASE(void) RingBuffer;

#define RING_BUFFER(elem_type) union { \
	RING_BUFFER_BASE(elem_type); \
	RingBuffer ring_buffer; \
}

#define RINGBUF_ASSERT_VALID(rbuf) do { \
	static_assert( \
		__builtin_types_compatible_p(RingBuffer, __typeof__((rbuf)->ring_buffer)), \
		"x->ring_buffer must be of RingBuffer type"); \
	static_assert( \
		__builtin_offsetof(__typeof__(*(rbuf)), ring_buffer) == 0, \
		"x->ring_buffer must be the first member in struct"); \
} while(0)

#define RBUF_ASSERT_CONSISTENT(rbuf) ({ \
	RINGBUF_ASSERT_VALID(rbuf); \
	auto _rbuf = rbuf; \
	assert(_rbuf->data != NULL); \
	assert(_rbuf->num_elements >= 0); \
	assert(_rbuf->num_elements <= _rbuf->capacity); \
	_rbuf; \
})

#define RINGBUF_CHECK_ELEMENT_TYPE(rbuf, elem) do { \
	static_assert( \
		__builtin_types_compatible_p(__typeof__(elem), __typeof__(*(rbuf)->data)), \
		"Ring buffer element type mismatch" \
	); \
} while(0)

#define RINGBUF_CAST_TO_BASE(rbuf) \
	&RBUF_ASSERT_CONSISTENT(rbuf)->ring_buffer

#define RING_BUFFER_INIT(backbuffer, num_elements) \
	{ .data = (backbuffer), .capacity = (num_elements), }

ringbuf_size_t _ringbuf_prepare_push(RingBuffer *rbuf) attr_nonnull_all attr_nodiscard;
ringbuf_size_t _ringbuf_prepare_pop(RingBuffer *rbuf) attr_nonnull_all attr_nodiscard;
ringbuf_size_t _ringbuf_prepare_peek(RingBuffer *rbuf, ringbuf_size_t ofs) attr_nonnull_all attr_nodiscard;

attr_nonnull_all
INLINE bool _ringbuf_is_full(RingBuffer *rbuf) {
	return rbuf->num_elements == rbuf->capacity;
}

attr_nonnull_all
INLINE bool _ringbuf_is_empty(RingBuffer *rbuf) {
	return rbuf->num_elements == 0;
}

#define ringbuf_is_full(rbuf) _ringbuf_is_full(RINGBUF_CAST_TO_BASE(rbuf))
#define ringbuf_is_empty(rbuf) _ringbuf_is_empty(RINGBUF_CAST_TO_BASE(rbuf))

#define ringbuf_push(rbuf, value) ({ \
	auto _rbuf = RBUF_ASSERT_CONSISTENT(rbuf); \
	RINGBUF_CHECK_ELEMENT_TYPE(_rbuf, value); \
	auto _idx = _ringbuf_prepare_push(&_rbuf->ring_buffer); \
	_rbuf->data[_idx] = value; \
})

#define ringbuf_pop(rbuf, default_value) ({ \
	auto _rbuf = RBUF_ASSERT_CONSISTENT(rbuf); \
	RINGBUF_CHECK_ELEMENT_TYPE(_rbuf, default_value); \
	auto _return_value = default_value; \
	auto _idx = _ringbuf_prepare_pop(&_rbuf->ring_buffer); \
	if(_idx >= 0) _return_value = _rbuf->data[_idx]; \
	_return_value; \
})

#define _ringbuf_peek_ptr(_rbuf, ofs) ({ \
	typeof(_rbuf->data) _ptr = NULL; \
	auto _idx = _ringbuf_prepare_peek(&_rbuf->ring_buffer, ofs); \
	if(_idx >= 0) _ptr = _rbuf->data + _idx; \
	_ptr; \
})

#define ringbuf_peek_ptr(rbuf, ofs) ({ \
	auto _rbuf = RBUF_ASSERT_CONSISTENT(rbuf); \
	_ringbuf_peek_ptr(_rbuf, ofs); \
})

#define _ringbuf_peek(_rbuf, ofs, default_value) ({ \
	RINGBUF_CHECK_ELEMENT_TYPE(_rbuf, default_value); \
	auto _ptr = _ringbuf_peek_ptr(_rbuf, ofs); \
	auto _return_value = default_value; \
	if(_ptr != NULL) _return_value = *_ptr; \
	_return_value; \
})

#define ringbuf_peek(rbuf, ofs, default_value) ({ \
	auto _rbuf = RBUF_ASSERT_CONSISTENT(rbuf); \
	_ringbuf_peek(_rbuf, ofs, default_value); \
})

#define ringbuf_rpeek_ptr(rbuf, ofs) ({ \
	auto _rbuf = RBUF_ASSERT_CONSISTENT(rbuf); \
	_ringbuf_peek_ptr(_rbuf, _rbuf->num_elements - ((ofs) + 1)); \
})

#define ringbuf_rpeek(rbuf, ofs, default_value) ({ \
	auto _rbuf = RBUF_ASSERT_CONSISTENT(rbuf); \
	_ringbuf_peek(_rbuf, _rbuf->num_elements - ((ofs) + 1), default_value); \
})
