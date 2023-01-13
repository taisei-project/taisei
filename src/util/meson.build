
util_deps = []

util_src = files(
    'assert.c',
    'crap.c',
    'env.c',
    'fbmgr.c',
    'fbpair.c',
    'fbutil.c',
    'geometry.c',
    'graphics.c',
    'io.c',
    'kvparser.c',
    'miscmath.c',
    'pngcruft.c',
    'rectpack.c',
    'strbuf.c',
    'stringops.c',
)

if is_developer_build
    util_src += files('debug.c')
endif

if dep_crypto.found()
    util_src += files('sha256_openssl.c')
else
    util_src += files('sha256.c')
endif

if host_machine.system() == 'windows'
    # NOTE: Even if we ever build this with something like Midipix, we'd
    # probably still want to use the winapi implementation of this here.
    util_src += files('platform_win32.c')
else
    # No have_posix check, it might just work.
    util_src += files('platform_posix.c')
endif

if dep_gamemode.found()
    util_src += files('gamemode.c')
else
    util_src += files('gamemode_stub.c')
endif

if not (float16_have_native_conversion or float16_have_rtlib_conversion)
    util_src += files('float16_fallback.c')
endif
