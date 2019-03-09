project('taisei', 'c',
    license : 'MIT',
    version : 'v1.3-dev',
    meson_version : '>=0.46.0',
    default_options : [
        'c_std=c11',

        # You may want to change these for a debug build dir
        'buildtype=release',
        'strip=true',
        'b_lto=true',
        'b_ndebug=if-release',
    ]
)

cc = meson.get_compiler('c')
python3 = import('python3').find_python()
macos_app_bundle = get_option('macos_bundle') and host_machine.system() == 'darwin'

subdir('scripts')

config = configuration_data()

taisei_c_args = [
    '-Wall',
    '-Wpedantic',
    '-Werror=assume',
    '-Werror=implicit-function-declaration',

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
    '-Wfloat-overflow-conversion',
    '-Wfloat-zero-conversion',
    '-Wfor-loop-analysis',
    '-Wformat-pedantic',
    '-Wformat-security',
    '-Wgcc-compat',
    '-Wgnu',
    '-Wignored-qualifiers',
    '-Wimplicit-fallthrough',
    '-Winit-self',
    '-Wlogical-op',
    '-Wmissing-prototypes',
    '-Wno-long-long',
    '-Wno-missing-braces',
    '-Wno-typedef-redefinition',
    '-Wnull-dereference',
    '-Wparentheses',
    '-Wshadow=compatible-local',
    '-Wsometimes-uninitialized',
    '-Wstrict-prototypes',
    '-Wtype-limits',
    '-Wunneeded-internal-declaration',
    '-Wunreachable-code',
    '-Wunreachable-code-loop-increment',
]

if get_option('b_pch') and meson.version().version_compare('<0.50.0')
    # Workaround for Meson bug: https://github.com/mesonbuild/meson/issues/4905
    taisei_c_args += ['-fpch-deps']
endif

taisei_c_args = cc.get_supported_arguments(taisei_c_args)

foreach arglist : [
        ['-msse', '-mfpmath=sse'],
    ]
    if cc.has_multi_arguments(arglist)
        taisei_c_args += arglist
    endif
endforeach

sm_check = run_command(check_submodules_command)

if sm_check.stdout() != ''
    foreach line : sm_check.stdout().strip().split('\n')
        warning(line)
    endforeach
endif

if sm_check.stderr() != ''
    warning('Submodule check completed with errors:\n@0@'.format(sm_check.stderr()))
endif

static = get_option('static') or (host_machine.system() == 'emscripten')

dep_freetype    = dependency('freetype2',                           required : true,  static : static)
dep_png         = dependency('libpng',         version : '>=1.5',   required : true,  static : static)
dep_sdl2        = dependency('sdl2',           version : '>=2.0.5', required : true,  static : static)
dep_sdl2_mixer  = dependency('SDL2_mixer',                          required : false, static : static)
dep_webp        = dependency('libwebp',        version : '>=0.5',   required : false, static : static)
dep_webpdecoder = dependency('libwebpdecoder', version : '>=0.5',   required : false, static : static)
dep_zip         = dependency('libzip',         version : '>=1.0',   required : false, static : static)
dep_zlib        = dependency('zlib',                                required : true,  static : static)
dep_crypto      = dependency('libcrypto',                           required : false, static : static)

dep_m           = cc.find_library('m', required : false)

dep_cglm        = subproject('cglm').get_variable('cglm_dep')
dep_glad        = subproject('glad').get_variable('glad_dep')

taisei_deps = [
    dep_cglm,
    dep_freetype,
    dep_m,
    dep_png,
    dep_sdl2,
    dep_zlib,
    # don't add glad here
]

if dep_webpdecoder.found()
    taisei_deps += dep_webpdecoder
elif dep_webp.found()
    warning('libwebpdecoder not found, will link against libwebp instead')
    taisei_deps += dep_webp
else
    error('libwebpdecoder or libwebp is required, neither found')
endif

if host_machine.system() == 'windows'
    taisei_deps += cc.find_library('shlwapi')
endif

if host_machine.system() == 'emscripten'
    package_data = false
    enable_zip = false
else
    package_data = get_option('package_data')
    enable_zip = get_option('enable_zip')
    package_data = (package_data == 'auto' ? enable_zip : package_data == 'true')
endif

if enable_zip
    if not dep_zip.found()
        error('ZIP support enabled but libzip not found')
    endif

    taisei_deps += dep_zip
endif

if package_data and not enable_zip
    error('ZIP support must be enabled for data packaging to work')
endif

config.set('TAISEI_BUILDCONF_USE_ZIP', taisei_deps.contains(dep_zip))

have_posix    =     cc.has_header_symbol('unistd.h', '_POSIX_VERSION')
have_vla      = not cc.has_header_symbol('stdlib.h', '__STDC_NO_VLA__')
have_complex  = not cc.has_header_symbol('stdlib.h', '__STDC_NO_COMPLEX__')
have_timespec =     cc.has_header_symbol('time.h',   'timespec_get')

assert(have_vla and have_complex, 'Your C implementation needs to support complex numbers and variable-length arrays.')

config.set('TAISEI_BUILDCONF_HAVE_TIMESPEC', have_timespec)
config.set('TAISEI_BUILDCONF_HAVE_INT128', cc.sizeof('__int128') == 16)
config.set('TAISEI_BUILDCONF_HAVE_LONG_DOUBLE', cc.sizeof('long double') > 8)
config.set('TAISEI_BUILDCONF_HAVE_POSIX', have_posix)
config.set('TAISEI_BUILDCONF_HAVE_MAX_ALIGN_T', cc.compiles('#include <stddef.h>\nmax_align_t i;', name : 'max_align_t test'))

prefer_relpath_systems = [
    'windows',
]

force_relpath_systems = [
    'emscripten',
]

if macos_app_bundle
    bundle_dir = 'Taisei.app'
    datadir = join_paths(bundle_dir, 'Contents', 'Resources')
    bindir = join_paths(bundle_dir, 'Contents', 'MacOS')
    config.set('TAISEI_BUILDCONF_RELATIVE_DATA_PATH', true)

    # arguments must be strings...
    meson.add_install_script(
        python3.path(),
        join_paths(meson.source_root(), 'scripts', 'macos-install-dylibs.py'),
        ':'.join(meson.get_cross_property('macos_lib_path', [])),
        ':'.join(meson.get_cross_property('macos_tool_path', [])),
        meson.get_cross_property('macos_tool_prefix', ''),
    )
else
    datadir = get_option('datadir')

    if force_relpath_systems.contains(host_machine.system())
        config.set('TAISEI_BUILDCONF_RELATIVE_DATA_PATH', true)
    elif get_option('install_relative') == 'auto'
        config.set('TAISEI_BUILDCONF_RELATIVE_DATA_PATH', prefer_relpath_systems.contains(host_machine.system()))
    else
        config.set('TAISEI_BUILDCONF_RELATIVE_DATA_PATH', get_option('install_relative') == 'true')
    endif

    if config.get('TAISEI_BUILDCONF_RELATIVE_DATA_PATH')
        bindir = '.'
    else
        bindir = get_option('bindir')
    endif
endif

if get_option('install_freedesktop') == 'auto'
    install_xdg = not config.get('TAISEI_BUILDCONF_RELATIVE_DATA_PATH')
else
    install_xdg = get_option('install_freedesktop') == 'true'
endif

if config.get('TAISEI_BUILDCONF_RELATIVE_DATA_PATH')
    data_path = 'data'
    doc_path = '.'  # Meson bug https://github.com/mesonbuild/meson/issues/4295
    xdg_path = 'freedesktop.org'

    # This is relative to SDL_GetBasePath()
    config.set_quoted('TAISEI_BUILDCONF_DATA_PATH', data_path)

    if macos_app_bundle
        # Actual installation path
        data_path = join_paths(datadir, data_path)

        # I don't know why would you do that, but more power to you
        xdg_path = join_paths(datadir, xdg_path)
    endif
else
    data_path = join_paths(datadir, 'taisei')
    config.set_quoted('TAISEI_BUILDCONF_DATA_PATH', join_paths(get_option('prefix'), data_path))
    doc_path = join_paths(datadir, 'doc', 'taisei')
    xdg_path = datadir
endif

is_debug_build = get_option('buildtype').startswith('debug')
is_developer_build = (get_option('developer') == 'auto' ? is_debug_build : get_option('developer') == 'true')

config.set('TAISEI_BUILDCONF_DEVELOPER', is_developer_build)
config.set('TAISEI_BUILDCONF_LOG_FATAL_MSGBOX', (
    host_machine.system() == 'windows' or
    host_machine.system() == 'darwin' or
    not is_developer_build
))
config.set('TAISEI_BUILDCONF_DEBUG_OPENGL', get_option('debug_opengl'))

install_docs = get_option('docs') and host_machine.system() != 'emscripten'

if host_machine.system() == 'windows'
    if install_docs
        custom_target('COPYING.txt',
            command : [eolconv_command, host_eol_style, '@INPUT@', '@OUTPUT@'],
            input : 'COPYING',
            output : 'COPYING.txt',
            install : true,
            install_dir : doc_path
        )
    endif

    if angle_enabled
        install_data(
            get_option('angle_libgles'),
            get_option('angle_libegl'),
            install_dir : join_paths(bindir, 'ANGLE'),
        )
    endif
else
    if install_docs
        install_data('COPYING', install_dir : doc_path)
    endif

    if angle_enabled
        error('install_angle is only available for Windows targets at the moment')
    endif
endif

systype = (have_posix ? 'POSIX (@0@)' : '@0@').format(host_machine.system())
systype = '@0@, @1@, @2@'.format(systype, host_machine.cpu_family(), host_machine.cpu())

if meson.is_cross_build()
    systype = '@0@ (cross-compiling)'.format(systype)
endif

version_deps = []

subdir('misc')
subdir('emscripten')
subdir('external')
subdir('resources')
subdir('doc')
subdir('xdg')
subdir('atlas')
subdir('src')

summary = '''

Summary:
    Taisei version:         @0@
    System type:            @1@
    Build type:             @2@

    Audio backends:         @3@  (default: @4@)
    Renderers:              @5@  (default: @6@)
    Shader translation:     @7@
    ZIP support:            @8@
    Package data:           @9@

    Relative install paths: @10@
    Prefix:                 @11@
    Executables:            @12@
    Data:                   @13@
    Documentation:          @14@
'''.format(
        taisei_version_string,
        systype,
        get_option('buildtype'),

        ', '.join(enabled_audio_backends),
        get_option('a_default'),
        ', '.join(enabled_renderers),
        get_option('r_default'),
        get_option('shader_transpiler'),
        enable_zip,
        package_data,

        config.get('TAISEI_BUILDCONF_RELATIVE_DATA_PATH'),
        get_option('prefix'),
        # the $ is intentional
        join_paths('$prefix', bindir),
        join_paths('$prefix', data_path),
        join_paths('$prefix', doc_path)
    )

message(summary)

run_command(postconf_command)
