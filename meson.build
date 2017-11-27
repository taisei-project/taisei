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

version_script = find_program('scripts/version.py')
version_command = [version_script, '--rootdir', meson.source_root(), '--fallback', version_fallback]

taisei_version_string = run_command(version_command, '{string}').stdout().strip()

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
    bindir = get_option('bindir')

    if get_option('install_relative') == 'auto'
        config.set('TAISEI_BUILDCONF_RELATIVE_DATA_PATH', host_machine.system() == 'windows')
    else
        config.set('TAISEI_BUILDCONF_RELATIVE_DATA_PATH', get_option('install_relative') == 'true')
    endif
endif

if config.get('TAISEI_BUILDCONF_RELATIVE_DATA_PATH')
    data_path = 'data'
    doc_path = '.'

    if macos_app_bundle
        # This is relative to SDL_GetBasePath(), which will return '/path/to/Taisei.app/Contents/Resources' for a macOS bundle.
        config.set_quoted('TAISEI_BUILDCONF_DATA_PATH', data_path)

        # Actual installation path
        data_path = join_paths(datadir, data_path)
    else
        # HACK
        rel_dpath = []
        foreach p : get_option('bindir').split('/')
            if p != ''
                rel_dpath += '..'
            endif
        endforeach
        rel_dpath += data_path

        config.set_quoted('TAISEI_BUILDCONF_DATA_PATH', format(join_paths(rel_dpath)))
    endif
else
    data_path = join_paths(datadir, 'taisei')
    config.set_quoted('TAISEI_BUILDCONF_DATA_PATH', join_paths(get_option('prefix'), data_path))
    doc_path = join_paths(datadir, 'doc', 'taisei')
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
    unix2dos = find_program('scripts/unix2dos.py')
    custom_target('COPYING.txt', command : [unix2dos, '@INPUT@', '@OUTPUT@'],
                  input : 'COPYING', output : 'COPYING.txt',
                  install : true, install_dir : doc_path)
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
        bindir,
        data_path,
        doc_path,
        get_option('buildtype'),
        taisei_version_string
    )

subdir('src')
subdir('resources')
subdir('misc')
subdir('doc')
subdir('xdg')

message(summary)

if macos_app_bundle
    dmg_command = find_program('scripts/macos-gen-dmg.sh')
    dmg_filename = 'Taisei-@0@-macOS.dmg'.format(taisei_version_string)
    dmg_target = run_target('dmg',
        command: [dmg_command, get_option('prefix'), join_paths(meson.build_root(), dmg_filename)],
    )
endif
