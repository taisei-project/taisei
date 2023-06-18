
r_gles30_src = files(
    'gles30.c',
)

if static_gles30
    r_gles30_src += files('fbcopy_fallback.c')
endif

r_gles30_deps = ['glescommon'] + r_glescommon_deps
r_gles30_libdeps = r_glescommon_libdeps

config.set('TAISEI_BUILDCONF_RENDERER_STATIC_GLES30', static_gles30)
