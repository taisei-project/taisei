
r_glcommon_src = files(
    'opengl.c',
    'debug.c',
)

headers_args = []

foreach header : gl_headers
    headers_args += ['-H', header]
endforeach

run_target('update-gl-header',
    command: [
        gen_opengl_header_command,
        join_paths(meson.current_source_dir(), '..'),
        files('opengl.h'),
        headers_args,
    ]
)
