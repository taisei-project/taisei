project('taisei', 'c',
    license : 'MIT',
    version : 'v1.3.1',
    meson_version : '>=0.48.0',
    default_options : [
        # Should really be c11, but gnu11 is a safer default because everything is terrible.
        'c_std=gnu11',

        'default_library=static',

        'libzip:enable_crypto=false',
        'libzip:enable_bzip2=false',

        'sdl2:use_haptic=disabled',
        'sdl2:use_power=disabled',
        'sdl2:use_render=disabled',
        'sdl2:use_sensor=disabled',

        # You may want to change these for a debug build dir
        'buildtype=release',
        'strip=true',
        'b_lto=true',
        'b_ndebug=if-release',
    ]
)

minimum_recommended_meson_version = '0.49.0'

if meson.version().version_compare('<@0@'.format(minimum_recommended_meson_version))
    warning('Old Meson version detected. Try upgrading to at least @0@ if the build fails.'.format(minimum_recommended_meson_version))
endif

is_debug_build = get_option('debug')
is_developer_build = (get_option('developer') == 'auto' ? is_debug_build : get_option('developer') == 'true')

cc = meson.get_compiler('c')
python3 = import('python3').find_python()
macos_app_bundle = get_option('macos_bundle') and host_machine.system() == 'darwin'

if cc.get_id() == 'emscripten'
    # emscripten LTO is really tricky
    # this logic should really be in Meson

    if get_option('b_lto')
        emscripten_global_link_args = [
            '-s', 'WASM_OBJECT_FILES=0',
            '--llvm-lto', '1',
            '-flto',
        ]
    else
        emscripten_global_link_args = []
    endif
    add_global_arguments(emscripten_global_link_args, language : 'c', native : false)
    add_global_arguments(emscripten_global_link_args, language : 'cpp', native : false)
    add_global_link_arguments(emscripten_global_link_args, language : 'c', native : false)
    add_global_link_arguments(emscripten_global_link_args, language : 'cpp', native : false)
endif

subdir('scripts')

config = configuration_data()

taisei_c_args = [
    '-Wall',
    '-Wpedantic',

    '-Werror=assume',
    '-Werror=implicit-function-declaration',
    '-Werror=incompatible-pointer-types',

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

    '-fexcess-precision=standard',
    '-fmerge-all-constants',
    '-fno-math-errno',
    '-fno-signaling-nans',
    '-fno-trapping-math',
]

deprecation_warnings = get_option('deprecation_warnings')

if deprecation_warnings == 'error'
    taisei_c_args += '-Werror=deprecated-declarations'
elif deprecation_warnings == 'no-error'
    taisei_c_args += '-Wno-error=deprecated-declarations'
elif deprecation_warnings == 'ignore'
    taisei_c_args += '-Wno-deprecated-declarations'
endif

if meson.version().version_compare('<0.50.0') and get_option('b_pch')
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

static = get_option('static') or ['emscripten', 'nx'].contains(host_machine.system())

dep_freetype    = dependency('freetype2',                           required : true,  static : static, fallback : ['freetype', 'freetype_dep'])
dep_opusfile    = dependency('opusfile',                            required : false, static : static, fallback : ['opusfile', 'opusfile_dep'])
dep_png         = dependency('libpng',         version : '>=1.5',   required : true,  static : static, fallback : ['libpng', 'png_dep'])
dep_sdl2        = dependency('sdl2',           version : '>=2.0.6', required : true,  static : static, fallback : ['sdl2', 'sdl2_dep'])
dep_sdl2_mixer  = dependency('SDL2_mixer',     version : '>=2.0.4', required : false, static : static, fallback : ['sdl2_mixer', 'sdl2_mixer_dep'])
dep_webp        = dependency('libwebp',        version : '>=0.5',   required : true,  static : static, fallback : ['libwebp', 'webpdecoder_dep'])
dep_webpdecoder = dependency('libwebpdecoder', version : '>=0.5',   required : false, static : static)
dep_zip         = dependency('libzip',         version : '>=1.2',   required : false, static : static, fallback : ['libzip', 'libzip_dep'])
dep_zlib        = dependency('zlib',                                required : true,  static : static, fallback : ['zlib', 'zlib_dep'])
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

if meson.version().version_compare('<0.49.0')
    wrap_mode_forcefallback = false
else
    wrap_mode_forcefallback = (get_option('wrap_mode') == 'forcefallback')
endif

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

if host_machine.system() == 'emscripten'
    package_data = false
    enable_zip = false
else
    package_data = get_option('package_data')
    enable_zip = get_option('enable_zip')
    package_data = (package_data == 'auto' ? enable_zip : package_data == 'true')
endif

if enable_zip
    assert(dep_zip.found(), 'ZIP support enabled but libzip not found')
    taisei_deps += dep_zip
endif

if package_data and not enable_zip
    error('ZIP support must be enabled for data packaging to work')
endif

config.set('TAISEI_BUILDCONF_USE_ZIP', taisei_deps.contains(dep_zip))

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

if host_machine.system() == 'emscripten'
    # Emscripten bug: https://github.com/emscripten-core/emscripten/issues/10072
    config.set('TAISEI_BUILDCONF_MALLOC_ALIGNMENT', 8)
    config.set('TAISEI_BUILDCONF_HAVE_MAX_ALIGN_T', false)
else
    malloc_alignment = cc.alignment('max_align_t', prefix : '#include <stddef.h>\n')
    config.set('TAISEI_BUILDCONF_MALLOC_ALIGNMENT', malloc_alignment)
    config.set('TAISEI_BUILDCONF_HAVE_MAX_ALIGN_T', malloc_alignment > 0)
endif

prefer_relpath_systems = [
    'windows',
]

force_relpath_systems = [
    'emscripten',
    'nx'
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

config.set('TAISEI_BUILDCONF_DEBUG', is_debug_build)
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

else
    if install_docs
        install_data('COPYING', install_dir : doc_path)
    endif

endif

if angle_enabled
    angle_dir = 'ANGLE'

    if host_machine.system() == 'windows'
        angle_path = angle_dir + '\\'
        gles_filename = angle_path + 'libGLESv2.dll'
        egl_filename = angle_path + 'libEGL.dll'
        # activate the Windows-Intel-ANGLE workaround
        config.set('TAISEI_BUILDCONF_WINDOWS_ANGLE_INTEL', 1)
    elif host_machine.system() == 'darwin'
        # this is relative to SDL_GetBasePath()
        # easiest way to traverse is to go up one dir
        angle_path = 'MacOS/' + angle_dir + '/'
        if not macos_app_bundle
            # if the user decides not to use the app bundle on macOS
            angle_path = 'lib/' + angle_dir + '/'
        endif
        gles_filename = '../' + angle_path + 'libGLESv2.dylib'
        egl_filename = '../' + angle_path + 'libEGL.dylib'
    else
        # FIXME: for Linux
        angle_path = 'lib/' + angle_dir + '/'
        gles_filename = '../' + angle_path + 'libGLESv2.so'
        egl_filename = '../' + angle_path + 'libEGL.so'
    endif

    # used in gles.c for determining what libraries to use
    config.set_quoted('TAISEI_BUILDCONF_ANGLE_GLES_PATH', gles_filename )
    config.set_quoted('TAISEI_BUILDCONF_ANGLE_EGL_PATH', egl_filename )

    # installs ANGLE to the paths specified
    install_data(
        get_option('angle_libgles'),
        get_option('angle_libegl'),
        install_dir : angle_path,
    )
endif

systype = (have_posix ? 'POSIX (@0@)' : '@0@').format(host_machine.system())
systype = '@0@, @1@, @2@'.format(systype, host_machine.cpu_family(), host_machine.cpu())

if meson.is_cross_build()
    systype = '@0@ (cross-compiling)'.format(systype)
endif

version_deps = []
bindist_deps = []

subdir('misc')
subdir('emscripten')
subdir('switch')
subdir('external')
subdir('resources')
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

summary = '''

Summary:
    Taisei version:         @0@
    System type:            @1@
    Build type:             @2@
    Developer mode:         @15@

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
        default_renderer,
        get_option('shader_transpiler'),
        enable_zip,
        package_data,

        config.get('TAISEI_BUILDCONF_RELATIVE_DATA_PATH'),
        get_option('prefix'),
        # the $ is intentional
        join_paths('$prefix', bindir),
        join_paths('$prefix', data_path),
        join_paths('$prefix', doc_path),
        is_developer_build
    )

message(summary)

run_command(postconf_command)
