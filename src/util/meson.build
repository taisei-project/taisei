
util_src = files(
    'assert.c',
    'crap.c',
    'env.c',
    'geometry.c',
    'graphics.c',
    'io.c',
    'kvparser.c',
    'miscmath.c',
    'pngcruft.c',
    'stringops.c',
)

sse42_src += files(
    'sse42.c',
)

if is_debug_build
    util_src += files('debug.c')
endif

if host_machine.system() == 'windows'
    util_src += files('windows_misc.c')
endif
