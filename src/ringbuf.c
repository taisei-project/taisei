/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2019, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2019, Andrei Alexeyev <akari@taisei-project.org>.
*/

#include "taisei.h"

#include "ringbuf.h"

static void push_index(RingBuffer *rbuf, ringbuf_size_t *idx) {
	++(*idx);

	if(*idx == rbuf->capacity) {
		*idx = 0;
	}
}

ringbuf_size_t _ringbuf_prepare_push(RingBuffer *rbuf) {
	ringbuf_size_t pidx = rbuf->head;

	if(_ringbuf_is_full(rbuf)) {
		push_index(rbuf, &rbuf->tail);
	} else {
		++rbuf->num_elements;
	}

	push_index(rbuf, &rbuf->head);
	return pidx;
}

ringbuf_size_t _ringbuf_prepare_pop(RingBuffer *rbuf) {
	ringbuf_size_t pidx = rbuf->tail;

	if(_ringbuf_is_empty(rbuf)) {
		return -1;
	}

	push_index(rbuf, &rbuf->tail);
	--rbuf->num_elements;

	return pidx;
}

ringbuf_size_t _ringbuf_prepare_peek(RingBuffer *rbuf, ringbuf_size_t ofs) {
	if(ofs >= rbuf->num_elements || ofs < 0) {
		return -1;
	}

	ringbuf_size_t pidx = rbuf->tail + ofs;

	if(pidx >= rbuf->num_elements) {
		pidx -= rbuf->num_elements;
	}

	return pidx;
}
