project('taisei', 'c',
    license : 'MIT',
    version : 'v1.2',
    meson_version : '>=0.45.0',
    default_options : [
        'c_std=c11',

        # You may want to change these for a debug build dir
        'buildtype=release',
        'strip=true',
        'b_lto=true',
        'b_ndebug=true',
    ]
)

ver_fb = get_option('version_fallback').strip()
version_fallback = ver_fb != '' ? ver_fb : meson.project_version()

common_taiseilib_args = [
    '--rootdir', meson.source_root(),
    '--fallback-version', version_fallback
]

common_taiseilib_defs = [
    '-DMESON_BUILD_TYPE=@0@'.format(get_option('buildtype')),
    '-DMESON_BUILD_ROOT=@0@'.format(meson.build_root()),
    '-DMESON_SOURCE_ROOT=@0@'.format(meson.source_root()),
]

scripts_dir = 'scripts'

version_script = find_program(join_paths(scripts_dir, 'version.py'))
version_command = [version_script, common_taiseilib_args]

preprocess_script = find_program(join_paths(scripts_dir, 'configure-file.py'))
preprocess_args = common_taiseilib_args + common_taiseilib_defs
preprocess_command = [preprocess_script, preprocess_args]

preprocessor = generator(preprocess_script,
    arguments: preprocess_args + ['@EXTRA_ARGS@', '--depfile', '@DEPFILE@', '@INPUT@', '@OUTPUT@'],
    output: '@BASENAME@',
    depfile: '@BASENAME@.d',
)

eolconv = find_program(join_paths(scripts_dir, 'eolconv.py'))
host_eol_style = host_machine.system() == 'windows' ? 'crlf' : 'lf'

taisei_version_result = run_command(version_command, '{string}')

if taisei_version_result.stderr() != ''
    warning(taisei_version_result.stderr().strip())
endif

if taisei_version_result.returncode() != 0
    error('Version script exited with code @0@'.format(
        taisei_version_result.returncode()
    ))
endif

taisei_version_string = taisei_version_result.stdout().strip()

config = configuration_data()

cc = meson.get_compiler('c')
python3 = import('python3').find_python()

taisei_c_warnargs = []
taisei_c_args = []

foreach flag : [
            '-Werror=implicit-function-declaration',
        ]
    if cc.has_argument(flag)
        taisei_c_args += flag
    endif
endforeach

foreach flag : [
            '-Wpedantic',
            '-Wparentheses',
            '-Wtype-limits',
            '-Wstrict-prototypes',
            '-Wlong-long',
            '-Winit-self',
            '-Wnull-dereference',
            '-Wformat-pedantic',
            '-Wgcc-compat',
            '-Wfloat-overflow-conversion',
            '-Wfloat-zero-conversion',
            '-Wfor-loop-analysis',
            '-Wimplicit-fallthrough',
            '-Wsometimes-uninitialized',
            '-Wunneeded-internal-declaration',
            '-Wunreachable-code',
            '-Wunreachable-code-loop-increment',
            '-Wgnu',
            '-Wcast-align',
            '-Wcast-align=strict',
            '-Wno-long-long',
        ]
    if cc.has_argument(flag)
        taisei_c_warnargs += flag
    endif
endforeach

taisei_c_args += taisei_c_warnargs

static = get_option('static')

dep_freetype    = dependency('freetype2',                           required : true,  static : static)
dep_png         = dependency('libpng',         version : '>=1.5',   required : true,  static : static)
dep_sdl2        = dependency('sdl2',           version : '>=2.0.5', required : true,  static : static)
dep_sdl2_mixer  = dependency('SDL2_mixer',                          required : false, static : static)
dep_webp        = dependency('libwebp',        version : '>=0.5',   required : false, static : static)
dep_webpdecoder = dependency('libwebpdecoder', version : '>=0.5',   required : false, static : static)
dep_zip         = dependency('libzip',         version : '>=1.0',   required : false, static : static)
dep_zlib        = dependency('zlib',                                required : true,  static : static)

dep_m           = cc.find_library('m',                              required : false)

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

if dep_sdl2_mixer.found() and get_option('enable_audio') != 'false'
    taisei_deps += dep_sdl2_mixer
elif get_option('enable_audio') == 'true'
    error('Audio support enabled but SDL2_mixer not found')
endif

if dep_zip.found() and get_option('package_data') != 'false'
    taisei_deps += dep_zip
elif get_option('package_data') == 'true'
    error('Data packaging enabled but libzip not found')
endif

config.set('TAISEI_BUILDCONF_USE_ZIP', taisei_deps.contains(dep_zip))

have_posix      =       cc.has_header_symbol('unistd.h',    '_POSIX_VERSION')
have_vla        = not   cc.has_header_symbol('unistd.h',    '__STDC_NO_VLA__')
have_complex    = not   cc.has_header_symbol('unistd.h',    '__STDC_NO_COMPLEX__')
have_backtrace  =       cc.has_header_symbol('execinfo.h',  'backtrace')
have_timespec   =       cc.has_header_symbol('time.h',      'timespec_get')

if not (have_vla and have_complex)
    error('Your C implementation needs to support complex numbers and variable-length arrays.')
endif

config.set('TAISEI_BUILDCONF_HAVE_TIMESPEC', have_timespec)
config.set('TAISEI_BUILDCONF_HAVE_INT128', cc.sizeof('__int128') == 16)
config.set('TAISEI_BUILDCONF_HAVE_LONG_DOUBLE', cc.sizeof('long double') > 8)

macos_app_bundle = get_option('macos_bundle') and host_machine.system() == 'darwin'

if macos_app_bundle
    bundle_dir = 'Taisei.app'
    datadir = join_paths(bundle_dir, 'Contents', 'Resources')
    bindir = join_paths(bundle_dir, 'Contents', 'MacOS')
    config.set('TAISEI_BUILDCONF_RELATIVE_DATA_PATH', true)

    # arguments must be strings...
    meson.add_install_script(
        python3.path(),
        join_paths(meson.source_root(), 'scripts', 'macos-install-dylibs.py'),
        get_option('macos_lib_path'),
        get_option('macos_tool_path'),
        get_option('macos_tool_prefix'),
    )
else
    datadir = get_option('datadir')

    if get_option('install_relative') == 'auto'
        config.set('TAISEI_BUILDCONF_RELATIVE_DATA_PATH', host_machine.system() == 'windows')
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
    doc_path = ''
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

config.set('TAISEI_BUILDCONF_DEBUG', is_debug_build)
config.set('TAISEI_BUILDCONF_LOG_ENABLE_BACKTRACE', is_debug_build and have_backtrace)
config.set('TAISEI_BUILDCONF_LOG_FATAL_MSGBOX', host_machine.system() == 'windows' or host_machine.system() == 'darwin')
config.set('TAISEI_BUILDCONF_DEBUG_OPENGL', get_option('debug_opengl'))

angle_enabled = get_option('install_angle')

if host_machine.system() == 'windows'
    custom_target('COPYING.txt',
        command : [eolconv, 'crlf', '@INPUT@', '@OUTPUT@'],
        input : 'COPYING',
        output : 'COPYING.txt',
        install : true,
        install_dir : doc_path
    )

    if angle_enabled
        install_data(
            get_option('angle_libgles'),
            get_option('angle_libegl'),
            install_dir : join_paths(bindir, 'ANGLE'),
        )
    endif
else
    install_data('COPYING', install_dir : doc_path)

    if angle_enabled
        error('install_angle is only available for Windows targets at the moment')
    endif
endif

systype = (have_posix ? 'POSIX (@0@)' : '@0@').format(host_machine.system())
systype = '@0@, @1@, @2@'.format(systype, host_machine.cpu_family(), host_machine.cpu())

summary = '''

Summary:
    Taisei version:         @9@

    System type:            @0@
    Audio enabled:          @1@
    Package data:           @2@

    Relative install paths: @3@
    Prefix:                 @4@
    Executables:            @5@
    Data:                   @6@
    Documentation:          @7@

    Build type:             @8@
'''.format(
        systype,
        taisei_deps.contains(dep_sdl2_mixer),
        taisei_deps.contains(dep_zip),
        config.get('TAISEI_BUILDCONF_RELATIVE_DATA_PATH'),
        get_option('prefix'),

        # the $ is intentional
        join_paths('$prefix', bindir),
        join_paths('$prefix', data_path),
        join_paths('$prefix', doc_path),

        get_option('buildtype'),
        taisei_version_string
    )

version_deps = []

subdir('misc')
subdir('external')
subdir('resources')
subdir('doc')
subdir('xdg')
subdir('scripts')
subdir('atlas')
subdir('src')

message(summary)

run_command(find_program(join_paths(scripts_dir, 'dump-build-options.py')), meson.build_root())
