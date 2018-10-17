
#define _CONV_IN_IS_FLOAT (_CONV_IN_MAX == 1.0f)
#define _CONV_OUT_IS_FLOAT (_CONV_OUT_MAX == 1.0f)

static void _CONV_FUNCNAME(
	size_t in_elements,
	size_t out_elements,
	size_t num_pixels,
	void *vbuf_in,
	void *vbuf_out,
	void *vdefault_pixel
) {
	_CONV_IN_TYPE *buf_in = vbuf_in;
	_CONV_OUT_TYPE *buf_out = vbuf_out;
	_CONV_OUT_TYPE *default_pixel = vdefault_pixel;

	for(size_t pixel = 0; pixel < num_pixels; ++pixel) {
		for(size_t element = 0; element < 4; ++element) {
			_CONV_OUT_TYPE fill;

			if(element < in_elements) {
				if(_CONV_IN_IS_FLOAT) {
					if(_CONV_OUT_IS_FLOAT) {
						fill = *buf_in++;
					} else {
						fill = (_CONV_OUT_TYPE)round(clamp(*buf_in++, 0, 1) * _CONV_OUT_MAX);
					}
				} else if(_CONV_OUT_IS_FLOAT) {
					fill = *buf_in++ * (1.0f / _CONV_IN_MAX);
				} else if(_CONV_OUT_MAX >= _CONV_IN_MAX) {
					fill = *buf_in++ * (_CONV_OUT_MAX / _CONV_IN_MAX);
				} else {
					fill = (_CONV_OUT_TYPE)round(*buf_in++ * ((float)_CONV_OUT_MAX / _CONV_IN_MAX));
				}
			} else {
				fill = default_pixel[element];
			}

			if(element < out_elements) {
				*buf_out++ = fill;
			}
		}
	}
}

#undef _CONV_FUNCNAME
#undef _CONV_IN_MAX
#undef _CONV_IN_TYPE
#undef _CONV_OUT_MAX
#undef _CONV_OUT_TYPE
#undef _CONV_IN_IS_FLOAT
#undef _CONV_OUT_IS_FLOAT
