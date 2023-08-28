
r_glescommon_deps = ['gl33'] + r_gl33_deps
r_glescommon_libdeps = r_gl33_libdeps

r_glescommon_src = files(
    'gles.c',
)

if config.get('TAISEI_BUILDCONF_HAVE_ANGLE', false)
    r_glescommon_src += files('angle_egl.c')
    r_glescommon_libdeps += dep_glad_egl
else
    r_glescommon_src += files('angle_egl_stub.c')
endif
