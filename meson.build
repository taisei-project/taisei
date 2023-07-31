project('taisei', 'c',
    license : 'MIT',
    # NOTE: See https://github.com/mesonbuild/meson/issues/11163
    version : run_command([
        files('scripts/version.py'),
        '--rootdir', meson.project_source_root(),
    ], check : true).stdout().strip(),
    meson_version : '>=0.63.0',
    default_options : [
        'c_std=gnu11',

        'default_library=static',
        'cglm:default_library=static',
        'freetype:default_library=static',
        'glslang:default_library=static',
        'libpng:default_library=static',
        'libwebp:default_library=static',
        'libzip:default_library=static',
        'libzstd:default_library=static',
        'ogg:default_library=static',
        'opus:default_library=static',
        'opusfile:default_library=static',
        'sdl2:default_library=static',
        'shaderc:default_library=static',
        'SPIRV-Cross:default_library=static',
        'vorbis:default_library=static',
        'zlib:default_library=static',

        'koishi:threadsafe=false',

        'cglm:werror=false',
        'cglm:install=false',
        'cglm:build_tests=false',

        'basis_universal:enable_ktx2=false',

        'libzip:bzip2=disabled',
        'libzip:lzma=disabled',
        'libzip:zstd=enabled',
        'libzip:enable_crypto=false',

        'libzstd:install_headers=false',
        'libzstd:install_libraries=false',
        'libzstd:install_pkgconfig=false',
        'libzstd:legacy_level=0',
        'libzstd:debug_level=0',
        'libzstd:bin_programs=false',
        'libzstd:bin_tests=false',
        'libzstd:bin_contrib=false',
        'libzstd:zlib=disabled',
        'libzstd:lzma=disabled',
        'libzstd:lz4=disabled',
        'libzstd:multi_thread=disabled',

        'sdl2:test=false',
        'sdl2:use_haptic=disabled',
        'sdl2:use_power=disabled',
        'sdl2:use_render=disabled',
        'sdl2:use_sensor=disabled',
        'sdl2:use_video_offscreen=disabled',
        'sdl2:use_video_vulkan=disabled',
        'sdl2:with_main=true',

        'freetype:brotli=disabled',
        'freetype:bzip2=disabled',
        'freetype:harfbuzz=disabled',
        'freetype:png=disabled',
        'freetype:tests=disabled',

        'opus:docs=disabled',
        'opus:extra-programs=disabled',
        'opus:tests=disabled',

        # You may want to change these for a debug build dir
        'buildtype=release',
        'strip=true',
        'b_lto=true',
        'b_ndebug=if-release',
    ]
)

taisei_version_string = meson.project_version()

is_debug_build = get_option('debug')
is_developer_build = (get_option('developer') == 'auto' ? is_debug_build : get_option('developer') == 'true')

cc = meson.get_compiler('c')
python = import('python').find_installation()

subdir('scripts')

config = configuration_data()

taisei_c_args = [
    '-Wall',

    '-Werror=assume',
    '-Werror=implicit-function-declaration',
    '-Werror=incompatible-pointer-types',
    '-Werror=return-type',

    #
    # Keep the rest sorted
    #

    '-Wabsolute-value',
    '-Wcast-align',
    '-Wcast-align=strict',
    '-Wcast-function-type',
    '-Wclobbered',
    '-Wduplicated-branches',
    '-Wduplicated-cond',
    '-Wfor-loop-analysis',
    '-Wformat-security',
    '-Wgcc-compat',
    '-Wignored-qualifiers',
    '-Winit-self',
    '-Wlogical-op',
    '-Wmissing-prototypes',
    '-Wno-declaration-after-statement',
    '-Wno-gnu-alignof-expression',
    '-Wno-gnu-folding-constant',
    '-Wno-ignored-optimization-argument',
    '-Wno-implicit-fallthrough',
    '-Wno-long-long',
    '-Wno-missing-braces',
    '-Wno-shadow',
    '-Wno-typedef-redefinition',
    '-Wnull-dereference',
    '-Wparentheses',
    '-Wsometimes-uninitialized',
    '-Wstrict-overflow=0',
    '-Wstrict-prototypes',
    '-Wtype-limits',
    '-Wunneeded-internal-declaration',
    '-Wunreachable-code',
    '-Wunreachable-code-loop-increment',

    '-fexcess-precision=standard',
    '-fmerge-all-constants',
    '-fno-math-errno',
    '-fno-signaling-nans',
    '-fno-trapping-math',
]

taisei_conversion_c_args = [
    # These are intended to enable "implicit conversion discards imaginary component" warnings
    # on clang. Those are gated by `-Wconversion`, which enables a ton of other stuff that we are
    # not ready for. So we need to undo that. Some of this stuff might be worth enabling later.

    # In GCC, -Wconversion also enables a bunch of stuff that can't be disabled individually, or so
    # it seems. It also doesn't have the warning we are actually interested in. That's why we test
    # for all of these flags at once: if the compiler accepts all of them, we assume clang-like
    # behavior.

    '-Wconversion',
    '-Wno-enum-conversion',
    '-Wno-float-conversion',
    '-Wfloat-overflow-conversion',
    '-Wno-implicit-int-conversion',
    '-Wno-implicit-int-float-conversion',
    '-Wno-implicit-float-conversion',
    '-Wno-shorten-64-to-32',
    '-Wno-sign-conversion',
    '-Wno-string-conversion',
]

deprecation_warnings = get_option('deprecation_warnings')

if deprecation_warnings == 'error'
    taisei_c_args += '-Werror=deprecated-declarations'
elif deprecation_warnings == 'no-error'
    taisei_c_args += '-Wno-error=deprecated-declarations'
elif deprecation_warnings == 'ignore'
    taisei_c_args += '-Wno-deprecated-declarations'
endif

taisei_c_args = cc.get_supported_arguments(taisei_c_args)

foreach arglist : [
        taisei_conversion_c_args,
        ['-msse', '-mfpmath=sse'],
    ]
    if cc.has_multi_arguments(arglist)
        taisei_c_args += arglist
    endif
endforeach

sm_check = run_command(check_submodules_command, check : false)

if sm_check.stdout() != ''
    foreach line : sm_check.stdout().strip().split('\n')
        warning(line)
    endforeach
endif

if sm_check.stderr() != ''
    warning('Submodule check completed with errors:\n@0@'.format(sm_check.stderr()))
endif

opt_vfs_zip = get_option('vfs_zip').require(host_machine.system() != 'emscripten',
    error_message : 'ZIP packages are not supported on Emscripten')

dep_freetype    = dependency('freetype2',                           required : true)
dep_png         = dependency('libpng',         version : '>=1.5',   required : true)
dep_sdl2        = dependency('sdl2',           version : '>=2.0.10',required : true)
dep_webp        = dependency('libwebp',        version : '>=0.5',   required : true)
dep_webpdecoder = dependency('libwebpdecoder', version : '>=0.5',   required : false)
dep_zlib        = dependency('zlib',                                required : true)
dep_zstd        = dependency('libzstd',        version : '>=1.4.0', fallback : ['libzstd', 'libzstd_dep'])
dep_zip         = dependency('libzip',         version : '>=1.5.0', required : opt_vfs_zip, allow_fallback : true)
dep_cglm        = dependency('cglm',           version : '>=0.7.8', required : true)
dep_crypto      = dependency('libcrypto',                           required : get_option('use_libcrypto'))
dep_gamemode    = dependency('gamemode',                            required : false)

dep_m           = cc.find_library('m', required : false)

dep_basisu_transcoder = subproject('basis_universal').get_variable('basisu_transcoder_dep')
dep_glad        = subproject('glad').get_variable('glad_dep')
dep_koishi      = subproject('koishi').get_variable('koishi_dep')

taisei_deps = [
    dep_basisu_transcoder,
    dep_cglm,
    dep_crypto,
    dep_freetype,
    dep_gamemode,
    dep_koishi,
    dep_m,
    dep_png,
    dep_sdl2,
    dep_zip,
    dep_zlib,
    dep_zstd,
    # don't add glad here
]

wrap_mode_forcefallback = (get_option('wrap_mode') == 'forcefallback')

if dep_webpdecoder.found() and not wrap_mode_forcefallback
    # distro libwebpdecoder
    taisei_deps += dep_webpdecoder
else
    # either distro libwebp, or libwebpdecoder from subproject
    taisei_deps += dep_webp
endif

if host_machine.system() == 'windows'
    taisei_deps += cc.find_library('shlwapi')
endif

package_data = (
    get_option('package_data')
        .require(dep_zip.found(),
            error_message : 'libzip not found or VFS ZIP support is disabled')
        .allowed()
)

config.set('TAISEI_BUILDCONF_USE_ZIP', dep_zip.found())

have_posix = cc.has_header_symbol('unistd.h', '_POSIX_VERSION')

# Feature test macros _SUCK_!
# On some platforms (FreeBSD, macOS, ...), defining _POSIX_C_SOURCE disables C11
# symbols, EVEN WHEN COMPILING IN C11 MODE! On macOS you can get around this
# with _DARWIN_C_SOURCE; on BSD you're screwed apparently. Some libCs require
# _POSIX_C_SOURCE for posix functionality (musl, glibc with __STRICT_ANSI__),
# others include everything by default, it's an inconsistent trash fire. So far
# the safest option seems to be to compile without strict conformance and only
# define _POSIX_C_SOURCE if we have to.
if have_posix
    if not cc.has_header_symbol('stdio.h', 'fileno')
        # No POSIX stuff by default? Then request it, and assume the libc isn't
        # braindead enough to hide the standard C11 APIs.
        # If it is then your system sucks.
        config.set('_POSIX_C_SOURCE', '200809L')
    endif

    # For good measure
    if host_machine.system() == 'darwin'
        config.set('_DARWIN_C_SOURCE', true)
    endif
endif

have_vla      = not cc.has_header_symbol('stdlib.h', '__STDC_NO_VLA__')
have_complex  = not cc.has_header_symbol('stdlib.h', '__STDC_NO_COMPLEX__')
have_timespec =     cc.has_function('timespec_get')

assert(have_vla and have_complex, 'Your C implementation needs to support complex numbers and variable-length arrays.')

config.set('TAISEI_BUILDCONF_HAVE_TIMESPEC', have_timespec)
config.set('TAISEI_BUILDCONF_HAVE_INT128', cc.sizeof('__int128') == 16)
config.set('TAISEI_BUILDCONF_HAVE_LONG_DOUBLE', cc.sizeof('long double') > 8)
config.set('TAISEI_BUILDCONF_HAVE_POSIX', have_posix)
config.set('TAISEI_BUILDCONF_HAVE_SINCOS', cc.has_function('sincos', dependencies : dep_m))

use_gnu_funcs = false
gnu_funcs = ['sincos', 'strtok_r', 'memrchr', 'memmem']

foreach f : gnu_funcs
    have = cc.has_function(f, dependencies : dep_m)
    config.set('TAISEI_BUILDCONF_HAVE_@0@'.format(f.to_upper()), have)
    use_gnu_funcs = have or use_gnu_funcs
endforeach

config.set('_GNU_SOURCE', use_gnu_funcs)

if host_machine.system() == 'emscripten'
    # Emscripten bug: https://github.com/emscripten-core/emscripten/issues/10072
    config.set('TAISEI_BUILDCONF_MALLOC_ALIGNMENT', 8)
    config.set('TAISEI_BUILDCONF_HAVE_MAX_ALIGN_T', false)
else
    malloc_alignment = cc.alignment('max_align_t', prefix : '#include <stddef.h>\n')
    config.set('TAISEI_BUILDCONF_MALLOC_ALIGNMENT', malloc_alignment)
    config.set('TAISEI_BUILDCONF_HAVE_MAX_ALIGN_T', malloc_alignment > 0)
endif

config.set('TAISEI_BUILDCONF_HAVE_BUILTIN_POPCOUNTLL', cc.has_function('__builtin_popcountll'))
config.set('TAISEI_BUILDCONF_HAVE_BUILTIN_POPCOUNT', cc.has_function('__builtin_popcount'))
config.set('TAISEI_BUILDCONF_HAVE_BUILTIN_AVAILABLE', cc.has_function('__builtin_available'))
config.set('TAISEI_BUILDCONF_HAVE_ALIGNED_ALLOC', cc.has_function('aligned_alloc'))
# XXX: meson thinks posix_memalign exists on Switch
config.set('TAISEI_BUILDCONF_HAVE_POSIX_MEMALIGN',
    host_machine.system() != 'nx' and cc.has_function('posix_memalign'))
config.set('TAISEI_BUILDCONF_HAVE_ALIGNED_MALLOC_FREE',
    cc.has_function('_aligned_malloc') and cc.has_function('_aligned_free'))

if dep_zip.found()
    if dep_zip.type_name() == 'internal'
        have_zip_compression_method_supported = dep_zip.version().version_compare('>=1.7.0')
    else
        have_zip_compression_method_supported = cc.has_function('zip_compression_method_supported', dependencies : dep_zip)
    endif
else
    have_zip_compression_method_supported = false
endif

config.set('TAISEI_BUILDCONF_HAVE_ZIP_COMPRESSION_METHOD_SUPPORTED', have_zip_compression_method_supported)

config.set('TAISEI_BUILDCONF_HAVE_ATTR_DESIGNATED_INIT', cc.compiles(
    'struct { int dummy; } __attribute__((designated_init)) x;',
    name : '__attribute__((designated_init))',
    args : ['-Wattributes', '-Werror']
))

config.set('TAISEI_BUILDCONF_HAVE_ATTR_MALLOC_WITH_ARGS', cc.compiles(
    'void myfree(void *p); void *myalloc() __attribute__((malloc(myfree, 1)));',
    name : '__attribute__((malloc(deallocator, index)))',
    args : ['-Wattributes', '-Werror']
))

config.set('TAISEI_BUILDCONF_HAVE_STATIC_ASSERT_WITHOUT_MSG', cc.compiles(
    '''
    #include <assert.h>
    static_assert(1, "with message");
    static_assert(1);
    ''',
    name : 'static_assert() without message',
    args : ['-Werror']
))

prefer_relpath_systems = [
    'windows',
]

force_relpath_systems = [
    'emscripten',
    'nx'
]

opt_install_macos_bundle = get_option('install_macos_bundle')
opt_install_macos_bundle = opt_install_macos_bundle.require(
    host_machine.system() == 'darwin',
    error_message : 'only supported on macOS')

prefer_relocatable = (
    host_machine.system() in (prefer_relpath_systems + force_relpath_systems) or
    opt_install_macos_bundle.allowed())

opt_install_relocatable = get_option('install_relocatable')
opt_install_relocatable = opt_install_relocatable.disable_auto_if(
    not prefer_relocatable)

opt_install_macos_bundle = opt_install_macos_bundle.require(
    opt_install_relocatable.allowed(),
    error_message : 'install_relocatable is required')

if host_machine.system() in force_relpath_systems and opt_install_relocatable.disabled()
    error('install_relocatable must be enabled on this platform')
endif

is_relocatable_install = opt_install_relocatable.allowed()
macos_app_bundle = opt_install_macos_bundle.allowed()

opt_install_freedesktop = (
    get_option('install_freedesktop')
        .disable_auto_if(is_relocatable_install or host_machine.system() == 'darwin')
        .require(not macos_app_bundle,
            error_message : 'installing freedesktop.org stuff into a macOS bundle makes no sense')
)

install_xdg = opt_install_freedesktop.allowed()

datadir = get_option('datadir')

if is_relocatable_install
    data_path = 'data'
    doc_path = '.'  # Meson bug https://github.com/mesonbuild/meson/issues/4295
    xdg_path = 'freedesktop.org'
    lib_path = '.'

    if macos_app_bundle
        bundle_dir = 'Taisei.app'

        bundle_datadir = join_paths('Contents', 'Resources')
        bundle_bindir = join_paths('Contents', 'MacOS')
        bundle_libdir = bundle_bindir

        datadir = join_paths(bundle_dir, bundle_datadir)
        bindir = join_paths(bundle_dir, bundle_bindir)
        libdir = join_paths(bundle_dir, bundle_libdir)

        # arguments must be strings...
        meson.add_install_script(
            python.path(),
            join_paths(meson.source_root(), 'scripts', 'macos-install-dylibs.py'),
            ':'.join(meson.get_cross_property('macos_lib_path', [])),
            ':'.join(meson.get_cross_property('macos_tool_path', [])),
            meson.get_cross_property('macos_tool_prefix', ''),
        )

        # Relative to SDL_GetBasePath() (bundle root)
        config.set_quoted('TAISEI_BUILDCONF_DATA_PATH', join_paths(bundle_datadir, data_path))
        config.set_quoted('TAISEI_BUILDCONF_LIB_PATH', join_paths(bundle_libdir))

        # Make paths prefix-relative for installation
        data_path = join_paths(datadir, data_path)
        lib_path = libdir
    else  # Relocatable; not macOS bundle
        bindir = '.'
        libdir = '.'

        # Relative to SDL_GetBasePath() (typically contains the executable)
        config.set_quoted('TAISEI_BUILDCONF_DATA_PATH', data_path)
        config.set_quoted('TAISEI_BUILDCONF_LIB_PATH', lib_path)
    endif
else  # Non-relocatable
    bindir = get_option('bindir')
    libdir = get_option('libdir')

    data_path = join_paths(datadir, 'taisei')
    lib_path = join_paths(libdir, 'taisei')
    doc_path = join_paths(datadir, 'doc', 'taisei')
    xdg_path = datadir

    # Static absolute paths
    config.set_quoted('TAISEI_BUILDCONF_DATA_PATH', join_paths(get_option('prefix'), data_path))
    config.set_quoted('TAISEI_BUILDCONF_LIB_PATH', join_paths(get_option('prefix'), lib_path))
endif

config.set('TAISEI_BUILDCONF_RELOCATABLE_INSTALL', is_relocatable_install)
config.set('TAISEI_BUILDCONF_DEBUG', is_debug_build)
config.set('TAISEI_BUILDCONF_DEVELOPER', is_developer_build)
config.set('TAISEI_BUILDCONF_LOG_FATAL_MSGBOX', (
    host_machine.system() == 'windows' or
    host_machine.system() == 'darwin' or
    not is_developer_build
))

if host_machine.system() == 'windows'
    custom_target('COPYING.txt',
        command : [eolconv_command, host_eol_style, '@INPUT@', '@OUTPUT@'],
        input : 'COPYING',
        output : 'COPYING.txt',
        install : true,
        install_dir : doc_path,
        install_tag : 'license',
    )
else
    install_data('COPYING', install_dir : doc_path, install_tag : 'license')
endif

if angle_enabled
    angle_dir = 'ANGLE'

    angle_libgles = get_option('angle_libgles')
    angle_libegl = get_option('angle_libegl')

    libgles_filename = angle_libgles.split('/')[-1]
    libegl_filename = angle_libegl.split('/')[-1]

    if build_machine.system() == 'windows'
        # Windows is terrible and has TWO valid path separators
        libgles_filename = angle_libgles.split('\\')[-1]
        libegl_filename = angle_libegl.split('\\')[-1]
    endif

    # Where to install the libs (prefix-relative)
    # Also used in doc/meson.build to install the license
    angle_install_path = join_paths(lib_path, angle_dir)

    # Where the game should look (either absolute or SDL_GetBasePath()-relative, depending on configuration)
    angle_config_path = join_paths(config.get_unquoted('TAISEI_BUILDCONF_LIB_PATH'), angle_dir)

    # use ANGLE for gles renderers by default
    config.set('TAISEI_BUILDCONF_HAVE_ANGLE', true)

    # used in gles.c for determining what libraries to use
    config.set_quoted('TAISEI_BUILDCONF_ANGLE_GLES_PATH', join_paths(angle_config_path, libgles_filename))
    config.set_quoted('TAISEI_BUILDCONF_ANGLE_EGL_PATH', join_paths(angle_config_path, libegl_filename))

    if host_machine.system() == 'windows'
        # Direct Intel iGPU users to the ANGLE fallback on Windows
        config.set('TAISEI_BUILDCONF_WINDOWS_ANGLE_INTEL', true)
    endif

    install_data(
        angle_libgles,
        angle_libegl,
        install_dir : angle_install_path,
        install_tag : 'runtime',
    )
endif

pathconv_args = ['--escape-backslashes']

if build_machine.system() == 'windows'
    pathconv_args += '--from-windows'
endif

if host_machine.system() == 'windows'
    pathconv_args += '--to-windows'
endif

foreach pathvar : [
    'TAISEI_BUILDCONF_ANGLE_EGL_PATH',
    'TAISEI_BUILDCONF_ANGLE_GLES_PATH',
    'TAISEI_BUILDCONF_DATA_PATH',
    'TAISEI_BUILDCONF_LIB_PATH',
]
    if config.has(pathvar)
        p = config.get_unquoted(pathvar)
        r = run_command(fix_path_command, pathconv_args, p, check : true)
        assert(r.returncode() == 0, 'path-correction script failed')
        p = r.stdout()
        # message('@0@ = @1@'.format(pathvar, p))
        config.set_quoted(pathvar, p)
    endif
endforeach

systype = (have_posix ? 'POSIX (@0@)' : '@0@').format(host_machine.system())
systype = '@0@, @1@, @2@'.format(systype, host_machine.cpu_family(), host_machine.cpu())

if meson.is_cross_build()
    systype = '@0@ (cross-compiling)'.format(systype)
endif

version_deps = []
bindist_deps = []

stages_live_reload = get_option('stages_live_reload')
if stages_live_reload
    assert(have_posix and host_machine.system() != 'emscripten',
        'Stage live reloading is not supported on this platform')
endif

use_testing_stages = is_developer_build

config.set('TAISEI_BUILDCONF_DYNSTAGE', stages_live_reload)
config.set('TAISEI_BUILDCONF_TESTING_STAGES', use_testing_stages)

# Stolen from Sway
# Compute the relative path used by compiler invocations.
source_root = meson.current_source_dir()
build_root = meson.project_build_root()
if build_machine.system() == 'windows'
    source_root = source_root.replace('\\', '/')
    build_root = build_root.replace('\\', '/')
endif
source_root = source_root.split('/')
build_root = build_root.split('/')

relative_dir_parts = []
i = 0
in_prefix = true
foreach p : build_root
    if i >= source_root.length() or not in_prefix or p != source_root[i]
        in_prefix = false
        relative_dir_parts += '..'
    endif
    i += 1
endforeach
i = 0
in_prefix = true
foreach p : source_root
    if i >= build_root.length() or not in_prefix or build_root[i] != p
        in_prefix = false
        relative_dir_parts += p
    endif
    i += 1
endforeach
relative_dir = join_paths(relative_dir_parts) + '/src/'

# Strip relative path prefixes from the code if possible, otherwise hide them.
if cc.has_argument('-fmacro-prefix-map=/prefix/to/hide=')
    add_project_arguments(
        '-fmacro-prefix-map=@0@='.format(relative_dir),
        language: 'c',
    )
else
    config.set_quoted('TAISEI_BUILDCONF_REL_SRC_DIR', relative_dir)
endif

subdir('misc')
subdir('emscripten')
subdir('switch')
subdir('resources')
subdir('external')
subdir('doc')
subdir('xdg')
subdir('atlas')
subdir('src')

if macos_app_bundle
    dmg_target = run_target('dmg',
        command : dmg_command,
        depends : bindist_deps,
    )
endif

if host_machine.system() == 'windows'
    nsis_target = run_target('nsis',
        command : nsis_command,
        depends : bindist_deps,
    )
endif

bindist_targets = ['zip', 'tar', 'txz', 'tgz', 'tbz']

foreach bindist_target : bindist_targets
    run_target(bindist_target,
        command : get_variable('@0@_command'.format(bindist_target)),
        depends : bindist_deps,
    )
endforeach

meson.add_devenv({
    'TAISEI_RES_PATH': meson.current_source_dir() / 'resources',
    'TAISEI_GL_DEBUG': '1',
    # TODO: Add support for this in libkoishi
    'ASAN_OPTIONS': 'detect_stack_use_after_return=false',
})

summary({
    'System type' : systype,
    'Build type' : get_option('buildtype'),
    'Developer mode' : is_developer_build,
}, section : 'Main', bool_yn : true)

summary({
    'Audio backends' : '@0@ (default: @1@)'.format(', '.join(enabled_audio_backends), default_audio_backend),
    'Rendering backends' : '@0@ (default: @1@)'.format(', '.join(enabled_renderers), default_renderer),
    'Shader translation' : shader_transpiler_enabled,
    'ZIP packages' : dep_zip.found(),
    'Stages live reload' : stages_live_reload,
}, section : 'Features', bool_yn : true)

summary({
    'Relocatable layout' : is_relocatable_install,
    'Data in ZIP packages' : package_data,
    'Bundle ANGLE libraries' : angle_enabled,
    'Prefix' : get_option('prefix'),
    'Executables' : join_paths('$prefix', bindir),
    'Data' : join_paths('$prefix', data_path),
    'Documentation' : join_paths('$prefix', doc_path),
}, section : 'Installation', bool_yn : true)

run_command(postconf_command, check : false)
