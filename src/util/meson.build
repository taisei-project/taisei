
util_src = files(
    'assert.c',
    'crap.c',
    'env.c',
    'fbpair.c',
    'geometry.c',
    'graphics.c',
    'io.c',
    'kvparser.c',
    'miscmath.c',
    'pixmap.c',
    'pngcruft.c',
    'rectpack.c',
    'stringops.c',
)

sse42_src += files(
    'sse42.c',
)

if is_debug_build
    util_src += files('debug.c')
endif

if host_machine.system() == 'windows'
    # NOTE: Even if we ever build this with something like Midipix, we'd
    # probably still want to use the winapi implementation of this here.
    util_src += files('platform_win32.c')
else
    # No have_posix check, it might just work.
    util_src += files('platform_posix.c')
endif
