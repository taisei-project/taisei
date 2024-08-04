
#include "taisei.h"

#include "test_renderer.h"
#include "resource/model.h"
#include "global.h"

typedef struct Vertex2D {
	vec2 pos;
	Color color;
} Vertex2D;

int main(int argc, char **argv) {
	test_init_renderer();

	const char *vert_shader_src =
		"#version 420\n"
		"layout(location = 0) in vec2 a_position;\n"
		"layout(location = 1) in vec4 a_color;\n"
		"layout(location = 0) out vec4 v_color;\n"
		"void main(void) {\n"
		"	gl_Position = vec4(a_position, 0, 1);\n"
		"	v_color = a_color;\n"
		"}\n";

	const char *frag_shader_src =
		"#version 420\n"
		"layout(location = 0) in vec4 v_color;\n"
		"layout(location = 0) out vec4 o_color;\n"
		"void main(void) {\n"
		"	o_color = v_color;\n"
		"}\n";

	ShaderObject *vert_obj = test_renderer_load_glsl(SHADER_STAGE_VERTEX, vert_shader_src);
	ShaderObject *frag_obj = test_renderer_load_glsl(SHADER_STAGE_FRAGMENT, frag_shader_src);
	ShaderProgram *prog = r_shader_program_link(2, (ShaderObject*[]) { vert_obj, frag_obj });
	r_shader_object_destroy(vert_obj);
	r_shader_object_destroy(frag_obj);

	VertexAttribSpec va_spec[] = {
		{ 2, VA_FLOAT, VA_CONVERT_FLOAT },
		{ 4, VA_FLOAT, VA_CONVERT_FLOAT },
	};

	VertexAttribFormat va_format[ARRAY_SIZE(va_spec)];
	r_vertex_attrib_format_interleaved(ARRAY_SIZE(va_spec), va_spec, va_format, 0);

	Vertex2D vertices[] = {
		{ { -1.0f, -1.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
		{ {  0.0f,  1.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
		{ {  1.0f, -1.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } },
	};

	VertexBuffer *vert_buf = r_vertex_buffer_create(sizeof(vertices), vertices);
	r_vertex_buffer_set_debug_label(vert_buf, "Triangle vertex buffer");

	VertexArray *vert_array = r_vertex_array_create();
	r_vertex_array_set_debug_label(vert_array, "Triangle vertex array");
	r_vertex_array_layout(vert_array, ARRAY_SIZE(va_format), va_format);
	r_vertex_array_attach_vertex_buffer(vert_array, vert_buf, 0);

	Model triangle = {
		.primitive = PRIM_TRIANGLES,
		.vertex_array = vert_array,
		.num_vertices = ARRAY_SIZE(vertices),
		.offset = 0,
	};

	r_shader_ptr(prog);

	while(!taisei_quit_requested()) {
		r_clear(BUFFER_COLOR, RGB(0, 0, 0), 1);
		r_draw_model_ptr(&triangle, 1, 0);
		events_poll(NULL, 0);
		video_swap_buffers();
	}

	r_vertex_buffer_destroy(vert_buf);
	r_vertex_array_destroy(vert_array);

	r_shader_program_destroy(prog);

	test_shutdown_renderer();
	return 0;
}
