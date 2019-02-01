
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

if is_developer_build
    util_src += files('debug.c')
endif

use_builtin_unreachable = cc.compiles('''
int main() {
    return 0;
    __builtin_unreachable();
}
''', name : '__builtin_unreachable() with -Werror', args : [ '-Wunreachable-code', '-Werror' ] )

use_designated_init = cc.compiles('''
struct { ; } __attribute__((designated_init)) x;
''', name : '__attribute__((designated_init)) with -Werror', args : [ '-Wattributes', '-Werror' ] )

if use_builtin_unreachable
    config.set('TAISEI_BUILDCONF_USE_BUILTIN_UNREACHABLE', true)
else
    config.set('TAISEI_BUILDCONF_USE_BUILTIN_UNREACHABLE', false)
endif

if use_designated_init
    config.set('TAISEI_BUILDCONF_USE_DESIGNATED_INIT', true)
else
    config.set('TAISEI_BUILDCONF_USE_DESIGNATED_INIT', false)
endif

if host_machine.system() == 'windows'
    # NOTE: Even if we ever build this with something like Midipix, we'd
    # probably still want to use the winapi implementation of this here.
    util_src += files('platform_win32.c')
else
    # No have_posix check, it might just work.
    util_src += files('platform_posix.c')
endif

subdir('pixmap_loaders')
util_src += pixmap_loaders_src
