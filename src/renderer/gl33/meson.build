
gl33_src = files(
    'core.c',
    'render_target.c',
    'shader_object.c',
    'shader_program.c',
    'texture.c',
    'vertex_buffer.c',
    'opengl.c',
)

run_target('gl33_update_glheader',
    command: [
        gen_opengl_header_command,
        meson.current_source_dir(),
        files('opengl.h'),
        '-f', 'glDebugMessageControlARB',
        '-f', 'glDebugMessageCallbackARB',
    ]
)
