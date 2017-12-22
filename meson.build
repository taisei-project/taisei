project('taisei', 'c',
    license : 'MIT',
    version : 'v1.1.0-9999',
    meson_version : '>=0.38.0',
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

taisei_version_result = run_command(version_command, '{string}')

if taisei_version_result.returncode() != 0
    error('Version script exited with code @0@\n\n@1@'.format(
        taisei_version_result.returncode(),
        taisei_version_result.stderr(),
    ))
endif

taisei_version_string = taisei_version_result.stdout().strip()

config = configuration_data()

cc = meson.get_compiler('c')
python3 = import('python3').find_python()

taisei_c_warnargs = []
taisei_c_args = []

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
        ]
    if cc.has_argument(flag)
        taisei_c_warnargs += flag
    endif
endforeach

taisei_c_args += taisei_c_warnargs

static = get_option('static')

dep_sdl2        = dependency('sdl2',    version : '>=2.0.5',    required : true,    static : static)
dep_sdl2_ttf    = dependency('SDL2_ttf',                        required : true,    static : static)
dep_sdl2_mixer  = dependency('SDL2_mixer',                      required : false,   static : static)
dep_zlib        = dependency('zlib',                            required : true,    static : static)
dep_png         = dependency('libpng',  version : '>=1.5',      required : true,    static : static)
dep_zip         = dependency('libzip',  version : '>=1.0',      required : false,   static : static)
dep_m           = cc.find_library('m',                          required : false)

taisei_deps = [dep_sdl2, dep_sdl2_ttf, dep_zlib, dep_png, dep_m]

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

if not (have_vla and have_complex)
    error('Your C implementation needs to support complex numbers and variable-length arrays.')
endif

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
        bindir = ''
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

if get_option('buildtype').startswith('debug')
    config.set('TAISEI_BUILDCONF_DEBUG', true)
    if have_backtrace
        config.set('TAISEI_BUILDCONF_LOG_ENABLE_BACKTRACE', true)
    endif
endif

if host_machine.system() == 'windows' or host_machine.system() == 'darwin'
        config.set('TAISEI_BUILDCONF_LOG_FATAL_MSGBOX', true)
endif

if host_machine.system() == 'windows'
    unix2dos = find_program(join_paths(scripts_dir, 'unix2dos.py'))
    custom_target('COPYING.txt',
        command : [unix2dos, '@INPUT@', '@OUTPUT@'],
        input : 'COPYING',
        output : 'COPYING.txt',
        install : true,
        install_dir : doc_path
    )
else
    install_data('COPYING', install_dir : doc_path)
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

subdir('src')
subdir('resources')
subdir('misc')
subdir('doc')
subdir('xdg')
subdir('scripts')

message(summary)

run_command(find_program(join_paths(scripts_dir, 'dump-build-options.py')), meson.build_root())
