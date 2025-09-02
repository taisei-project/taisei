/*
 * This software is licensed under the terms of the MIT License.
 * See COPYING for further information.
 * ---
 * Copyright (c) 2011-2025, Lukas Weber <laochailan@web.de>.
 * Copyright (c) 2012-2025, Andrei Alexeyev <akari@taisei-project.org>.
 */

#include "state.h"

#include "backend.h"

#define RSTATE_STACK_SIZE 16

static struct {
	RendererStateRollback *head;
	RendererStateRollback stack[RSTATE_STACK_SIZE];
} _r_state;

void _r_state_init(void) {
	_r_state = (typeof(_r_state)) {};
}

void _r_state_shutdown(void) {

}

#define S (*_r_state.head)
#define B (_r_backend.funcs)
#define TAINT(db, code) do {\
	if(_r_state.head && !(S.dirty_bits & (db))) { \
			S.dirty_bits |= (db); \
			do { code } while(0); \
		} \
	} while(0);
// #define RESTORE(db) if(S.dirty_bits & (db)) log_debug(#db); if(S.dirty_bits & (db))
#define RESTORE(db) if(S.dirty_bits & (db))

void r_state_push(void) {
	if(_r_state.head) {
		assert(_r_state.head < _r_state.stack + RSTATE_STACK_SIZE - 1);
		_r_state.head++;
	} else {
		_r_state.head = _r_state.stack;
	}

	// memset(_r_state.head, 0, sizeof(*_r_state.head));
	_r_state.head->dirty_bits = 0;
}

void r_state_pop(void) {
	assert(_r_state.head >= _r_state.stack);

	RESTORE(RSTATE_CAPABILITIES) {
		B.capabilities(S.capabilities);
	}

	RESTORE(RSTATE_COLOR) {
		B.color4(S.color.r, S.color.g, S.color.b, S.color.a);
	}

	RESTORE(RSTATE_BLENDMODE) {
		B.blend(S.blend_mode);
	}

	RESTORE(RSTATE_CULLMODE) {
		B.cull(S.cull_mode);
	}

	RESTORE(RSTATE_DEPTHFUNC) {
		B.depth_func(S.depth_func);
	}

	RESTORE(RSTATE_SHADER) {
		B.shader(S.shader);
	}

	RESTORE(RSTATE_SHADER_UNIFORMS) {
		// TODO
	}

	RESTORE(RSTATE_RENDERTARGET) {
		B.framebuffer(S.framebuffer);
	}

	RESTORE(RSTATE_SCISSOR) {
		B.scissor(S.scissor);
	}

	if(_r_state.head == _r_state.stack) {
		_r_state.head = NULL;
	} else {
		_r_state.head--;
	}
}

void _r_state_touch_capabilities(void) {
	TAINT(RSTATE_CAPABILITIES, {
		S.capabilities = B.capabilities_current();
	});
}

void _r_state_touch_color(void) {
	TAINT(RSTATE_COLOR, {
		S.color = *B.color_current();
	});
}

void _r_state_touch_blend_mode(void) {
	TAINT(RSTATE_BLENDMODE, {
		S.blend_mode = B.blend_current();
	});
}

void _r_state_touch_cull_mode(void) {
	TAINT(RSTATE_CULLMODE, {
		S.cull_mode = B.cull_current();
	});
}
void _r_state_touch_depth_func(void) {
	TAINT(RSTATE_DEPTHFUNC, {
		S.depth_func = B.depth_func_current();
	});
}

void _r_state_touch_shader(void) {
	TAINT(RSTATE_SHADER, {
		S.shader = B.shader_current();
	});
}

void _r_state_touch_uniform(Uniform *uniform) {
	// TODO
}

void _r_state_touch_framebuffer(void) {
	TAINT(RSTATE_RENDERTARGET, {
		S.framebuffer = B.framebuffer_current();
	});
}

void _r_state_touch_scissor(void) {
	TAINT(RSTATE_SCISSOR, {
		B.scissor_current(&S.scissor);
	});
}
