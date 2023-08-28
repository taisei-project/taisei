
r_glcommon_src = files(
    'debug.c',
    'opengl.c',
    'shaders.c',
    'texture.c',
)

r_glcommon_libdeps = []

if not static_gles30
    r_glcommon_libdeps += dep_glad_gl
endif
