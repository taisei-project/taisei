project('taisei', 'c', license : 'MIT', version : 'v1.1.0-9999',
        meson_version : '>=0.38.0',
        default_options : ['c_std=c11', 'b_lto=true'])

config = configuration_data()

cc = meson.get_compiler('c')

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
            '-Wgnu'
        ]
    if cc.has_argument(flag)
        taisei_c_args += flag
    endif
endforeach

static = get_option('static')

dep_sdl2 = dependency('sdl2', version : '>=2.0.5', required : true, static : static)
dep_sdl2_ttf = dependency('SDL2_ttf', required : true, static: static)
dep_zlib = dependency('zlib', required : true, static: static)
dep_png = dependency('libpng', version : '>=1.5', required : true, static: static)
dep_m = cc.find_library('m', required : false)

taisei_deps = [dep_sdl2, dep_sdl2_ttf, dep_zlib, dep_png, dep_m]

if host_machine.system() == 'windows'
    taisei_deps += cc.find_library('shlwapi')
endif

dep_sdl2_mixer = dependency('SDL2_mixer', required : false, static: static)
if dep_sdl2_mixer.found() and get_option('enable_audio') != 'false'
    taisei_deps += dep_sdl2_mixer
elif get_option('enable_audio') == 'true'
    error('Audio support enabled but SDL2_mixer not found')
endif

dep_zip = dependency('libzip', version : '>=1.0', required : false, static: static)
if dep_zip.found() and get_option('package_data') != 'false'
    taisei_deps += dep_zip
elif get_option('package_data') == 'true'
    error('Data packaging enabled but libzip not found')
endif

config.set('DISABLE_ZIP', not taisei_deps.contains(dep_zip))

have_posix = cc.has_header_symbol('unistd.h', '_POSIX_VERSION')
have_vla = not cc.has_header_symbol('unistd.h', '__STDC_NO_VLA__')
have_complex = not cc.has_header_symbol('unistd.h', '__STDC_NO_COMPLEX__')
have_backtrace = cc.has_header_symbol('execinfo.h', 'backtrace')

if not (have_vla and have_complex)
    error('Your C implementation needs to support complex numbers and variable-length arrays.')
endif


if get_option('install_relative') == 'auto'
    config.set('RELATIVE', host_machine.system() == 'windows' or host_machine.system() == 'darwin')
else
    config.set('RELATIVE', get_option('install_relative') == 'true')
endif

if config.get('RELATIVE')
    if host_machine.system() == 'darwin'
        data_path = join_paths(get_option('datadir'), 'data')
        config.set('DATA_PATH', '"@0@"'.format(join_paths('../..', data_path)))
    else
        data_path = 'data'

        # HACK
        rel_dpath = []
        foreach p : get_option('bindir').split('/')
            if p != ''
                rel_dpath += '..'
            endif
        endforeach
        rel_dpath += data_path

        config.set('DATA_PATH', '"@0@"'.format(join_paths(rel_dpath)))
    endif
    doc_path = ''
else
    data_path = join_paths(get_option('datadir'), 'taisei')
    config.set('DATA_PATH', '"@0@"'.format(join_paths(get_option('prefix'), data_path)))
    doc_path = join_paths(get_option('datadir'), 'doc', 'taisei')
endif


systype = have_posix ? 'POSIX' : host_machine.system()

if get_option('buildtype').startswith('debug')
    add_global_arguments('-DDEBUG', language: 'c')
    if have_backtrace
        add_global_arguments('-DLOG_ENABLE_BACKTRACE', language : 'c')
    endif
endif

if host_machine.system() == 'windows' or host_machine.system() == 'darwin'
    add_global_arguments('-DLOG_FATAL_MSGBOX', language : 'c')
endif

if host_machine.system() == 'windows'
    unix2dos = find_program('scripts/unix2dos.py')
    custom_target('COPYING.txt', command : [unix2dos, '@INPUT@', '@OUTPUT@'],
                  input : 'COPYING', output : 'COPYING.txt',
                  install : true, install_dir : doc_path)
else
    install_data('COPYING', install_dir : doc_path)
endif

summary = '''

Summary:
    System type:    @0@
    Audio enabled:  @1@
    Package data:   @2@

    Relative install paths: @3@
    Prefix:         @4@
    Executables:    @5@
    Data:           @6@
    Documentation:  @7@

    Build type:     @8@
'''.format(
        systype,
        taisei_deps.contains(dep_sdl2_mixer),
        taisei_deps.contains(dep_zip),
        config.get('RELATIVE'),
        get_option('prefix'),
        get_option('bindir'),
        data_path,
        doc_path,
        get_option('buildtype')
    )

message(summary)

subdir('src')
subdir('resources')
subdir('misc')
subdir('doc')
subdir('xdg')
