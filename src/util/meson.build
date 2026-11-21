
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
    'pathutil.c',
    'random.c',
    'rectpack.c',
    'sort_r.c',
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
