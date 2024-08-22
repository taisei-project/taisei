
#include "global.h"
#include "renderer/common/models.h"
#include "test_renderer.h"

int main(int argc, char **argv) {
	test_init_renderer();
	r_models_init();
	time_init();

	FPSCounter fps = {};

	const char *vert_shader_src =
		"#version 420\n"
		"layout(location = 0) in vec3 a_position;\n"
		"layout(location = 1) in vec2 a_uv;\n"
		"layout(location = 2) in vec3 a_normal;\n"
		"layout(location = 0) out vec2 v_uv;\n"
		"layout(location = 1) out vec3 v_normal;\n"
		"layout(location = 2) out vec3 v_fragPos;\n"
		"uniform mat4 r_modelViewMatrix;"
		"uniform mat4 r_projectionMatrix;"
		"void main(void) {\n"
		"	gl_Position = r_projectionMatrix * r_modelViewMatrix * vec4(a_position, 1);\n"
		"	v_fragPos = (r_modelViewMatrix * vec4(a_position, 1)).xyz;"
		"	v_uv = a_uv;\n"
		"	v_normal = mat3(transpose(inverse(r_modelViewMatrix))) * a_normal;\n"
		"}\n";

	const char *frag_shader_src =
		"#version 420\n"
		"layout(location = 0) in vec2 v_uv;\n"
		"layout(location = 1) in vec3 v_normal;\n"
		"layout(location = 2) in vec3 v_fragPos;\n"
		"layout(location = 0) out vec4 o_color;\n"
		"uniform sampler2D u_sampler;\n"
		"uniform vec3 u_lightPos;\n"
		"uniform vec3 u_lightColor;\n"
		"void main(void) {\n"
		"	vec3 obj_color = (0.5 + 0.5 * vec3(v_uv, 0)) * texture(u_sampler, v_uv).rgb;\n"
		"	vec3 normal = normalize(v_normal);\n"
		"	vec3 lightDir = normalize(u_lightPos - v_fragPos);\n"
		"	vec3 diffuse = (0.1 + max(dot(normal, lightDir), 0.0) * u_lightColor) * obj_color;\n"
		"	o_color = vec4(diffuse, 1);\n"
		"}\n";

	ShaderObject *vert_obj = test_renderer_load_glsl(SHADER_STAGE_VERTEX, vert_shader_src);
	ShaderObject *frag_obj = test_renderer_load_glsl(SHADER_STAGE_FRAGMENT, frag_shader_src);
	ShaderProgram *prog = r_shader_program_link(2, (ShaderObject*[]) { vert_obj, frag_obj });
	r_shader_object_destroy(vert_obj);
	r_shader_object_destroy(frag_obj);

	Model cube = test_renderer_load_iqm("cube.iqm");
	Texture *tex = test_renderer_load_texture("texture0.webp");

	r_shader_ptr(prog);
	auto u_sampler = r_shader_uniform(prog, "u_sampler");
	auto u_lightPos = r_shader_uniform(prog, "u_lightPos");
	auto u_lightColor = r_shader_uniform(prog, "u_lightColor");

	r_uniform_vec3(u_lightPos, 1, 0.5, 2);
	// r_uniform_vec3(u_lightColor, 1, 0.1, 0.2);
	r_uniform_vec3(u_lightColor, 1, 1, 1);

	hrtime_t last_time = time_get();
	hrtime_t last_print_time = last_time;

	r_mat_proj_perspective(90, SCREEN_W/(float)SCREEN_H, 0, 3);
	r_mat_mv_translate(0, 0, -2.5);
	r_mat_mv_rotate(-1.0, 1, 0, 0);

	r_enable(RCAP_CULL_FACE);
	r_cull(CULL_BACK);
	r_blend(BLEND_NONE);

	float a = 0;

	while(!taisei_quit_requested()) {
		r_mat_mv_push();
		r_mat_mv_rotate(a, 0, 0, 1);
		r_clear(BUFFER_COLOR, RGB(0, 0, 0), 1);
		r_uniform_sampler(u_sampler, tex);
		r_draw_model_ptr(&cube, 1, 0);
		r_mat_mv_pop();

		events_poll(NULL, 0);
		video_swap_buffers();

		fpscounter_update(&fps);

		hrtime_t t = time_get();
		if(t - last_print_time > HRTIME_RESOLUTION) {
			last_print_time = t;
			log_debug("%.02f FPS", fps.fps);
		}

		a += (t - last_time) / (double)HRTIME_RESOLUTION;
		last_time = t;
	}

	r_shader_program_destroy(prog);

	time_shutdown();
	r_models_shutdown();
	test_shutdown_renderer();
	return 0;
}
