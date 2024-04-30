
r_gl33_src = files(
    'common_buffer.c',
    'framebuffer.c',
    'gl33.c',
    'index_buffer.c',
    'shader_object.c',
    'shader_program.c',
    'texture.c',
    'vertex_array.c',
    'vertex_buffer.c',
)

if static_gles30
    r_gl33_src += files('framebuffer_async_read_fake.c')
else
    r_gl33_src += files('framebuffer_async_read.c')
endif

r_gl33_deps = ['glcommon']
r_gl33_libdeps = r_glcommon_libdeps
