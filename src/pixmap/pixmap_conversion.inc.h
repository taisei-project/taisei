
#include "util/macrohax.h"

#define _CONV_IN_IS_FLOAT ((float)_CONV_IN_MAX == 1.0f)
#define _CONV_OUT_IS_FLOAT ((float)_CONV_OUT_MAX == 1.0f)

#define _CONV_VALUE_FUNC \
	MACROHAX_CONCAT(_CONV_FUNCNAME, _convert_value)

#define _CONV_IMPL_FUNC \
	MACROHAX_CONCAT(_CONV_FUNCNAME, _impl)

#define _CONV_DISPATCH_1_FUNC \
	_CONV_FUNCNAME

#define _CONV_DISPATCH_2_FUNC \
	MACROHAX_CONCAT(_CONV_FUNCNAME, _dispatch_2)

static _CONV_OUT_TYPE _CONV_VALUE_FUNC(_CONV_IN_TYPE val) {
	if(_CONV_IN_IS_FLOAT) {
		if(_CONV_OUT_IS_FLOAT) {
			return val;
		} else {
			return (_CONV_OUT_TYPE)roundf(clampf(val, 0.0f, 1.0f) * (float)_CONV_OUT_MAX);
		}
	} else if(_CONV_OUT_IS_FLOAT) {
		return val * (1.0f / (float)_CONV_IN_MAX);
	} else if(_CONV_OUT_MAX == _CONV_IN_MAX) {
		return val;
	} else if(_CONV_OUT_MAX > _CONV_IN_MAX) {
		return val * (_CONV_OUT_MAX / _CONV_IN_MAX);
	} else {
		// TODO use fixed point math
		return (_CONV_OUT_TYPE)roundf(val * ((float)_CONV_OUT_MAX / (float)_CONV_IN_MAX));
	}
}

static void _CONV_IMPL_FUNC(
	size_t in_elements,
	size_t out_elements,
	size_t num_pixels,
	void *vbuf_in,
	void *vbuf_out,
	int swizzle[4]
) {
	ASSUME(in_elements > 0);
	ASSUME(in_elements <= 4);
	ASSUME(out_elements > 0);
	ASSUME(out_elements <= 4);
	ASSUME(num_pixels > 0);

	_CONV_IN_TYPE *buf_in = vbuf_in;
	_CONV_OUT_TYPE *buf_out = vbuf_out;
	_CONV_OUT_TYPE *buf_out_end = buf_out + out_elements * num_pixels;

	size_t i = 0;

	if(swizzle) {
		_CONV_IN_TYPE swizzle_buf[] = {
			// R  G  B  A             const-0  const-1
			   0, 0, 0, _CONV_IN_MAX, 0,       _CONV_IN_MAX
		};

		do {
			i = 0;
			do {
				swizzle_buf[i] = _CONV_VALUE_FUNC(buf_in[i]);
			} while(++i < in_elements);

			i = 0;
			do {
				buf_out[i] = swizzle_buf[swizzle[i]];
			} while(++i < out_elements);

			buf_out += out_elements;
			buf_in += in_elements;
		} while(buf_out < buf_out_end);
	} else if(out_elements <= in_elements) {
		do {
			i = 0;
			do {
				buf_out[i] = _CONV_VALUE_FUNC(buf_in[i]);
			} while(++i < out_elements);

			buf_out += out_elements;
			buf_in += in_elements;
		} while(buf_out < buf_out_end);
	} else {
		assume(out_elements > in_elements);
		_CONV_OUT_TYPE default_buf[] = { 0, 0, 0, _CONV_OUT_MAX };

		do {
			i = 0;
			do {
				buf_out[i] = _CONV_VALUE_FUNC(buf_in[i]);
			} while(++i < in_elements);

			// NOTE: don't reset i
			do {
				buf_out[i] = default_buf[i];
			} while(++i < out_elements);

			buf_out += out_elements;
			buf_in += in_elements;
		} while(buf_out < buf_out_end);
	}
}

static void _CONV_DISPATCH_2_FUNC(
	size_t in_elements,
	size_t out_elements,
	size_t num_pixels,
	void *vbuf_in,
	void *vbuf_out,
	int swizzle[4]
) {
	switch(out_elements) {
		case 1: _CONV_IMPL_FUNC(in_elements, 1, num_pixels, vbuf_in, vbuf_out, swizzle); return;
		case 2: _CONV_IMPL_FUNC(in_elements, 2, num_pixels, vbuf_in, vbuf_out, swizzle); return;
		case 3: _CONV_IMPL_FUNC(in_elements, 3, num_pixels, vbuf_in, vbuf_out, swizzle); return;
		case 4: _CONV_IMPL_FUNC(in_elements, 4, num_pixels, vbuf_in, vbuf_out, swizzle); return;
		default: UNREACHABLE;
	}
}

static void _CONV_DISPATCH_1_FUNC(
	size_t in_elements,
	size_t out_elements,
	size_t num_pixels,
	void *vbuf_in,
	void *vbuf_out,
	int swizzle[4]
) {
	switch(in_elements) {
		case 1: _CONV_DISPATCH_2_FUNC(1, out_elements, num_pixels, vbuf_in, vbuf_out, swizzle); return;
		case 2: _CONV_DISPATCH_2_FUNC(2, out_elements, num_pixels, vbuf_in, vbuf_out, swizzle); return;
		case 3: _CONV_DISPATCH_2_FUNC(3, out_elements, num_pixels, vbuf_in, vbuf_out, swizzle); return;
		case 4: _CONV_DISPATCH_2_FUNC(4, out_elements, num_pixels, vbuf_in, vbuf_out, swizzle); return;
		default: UNREACHABLE;
	}
}

#undef _CONV_DISPATCH_1_FUNC
#undef _CONV_DISPATCH_2_FUNC
#undef _CONV_FUNCNAME
#undef _CONV_IMPL_FUNC
#undef _CONV_IN_IS_FLOAT
#undef _CONV_IN_MAX
#undef _CONV_IN_TYPE
#undef _CONV_OUT_IS_FLOAT
#undef _CONV_OUT_MAX
#undef _CONV_OUT_TYPE
#undef _CONV_VALUE_FUNC
