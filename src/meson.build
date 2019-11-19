
version_deps += custom_target('version information',
    command : [preprocess_command, '@INPUT@', '@OUTPUT@'],
    build_by_default : true,
    build_always_stale : true,
    input : 'version_auto.c.in',
    output : 'version_auto.c',
)

if host_machine.system() == 'windows'
    winmod = import('windows')

    rcpath = join_paths(meson.current_build_dir(), 'taisei.rc')
    icons_dir = join_paths(meson.source_root(), 'misc', 'icons')
    icon_main = join_paths(icons_dir, 'taisei.ico')
    icon_replay = join_paths(icons_dir, 'taisei-replay.ico')
    manifest = join_paths(meson.current_source_dir(), 'taisei.manifest')

    rcdefs = [
        '-DICON_MAIN=@0@'.format(icon_main),
        '-DICON_REPLAY=@0@'.format(icon_replay),
        '-DMANIFEST=@0@'.format(manifest),
    ]

    if is_debug_build
        rcdefs += ['-DBUILDTYPE_DEFINE=#define DEBUG_BUILD']
    else
        rcdefs += ['-DBUILDTYPE_DEFINE=#define RELEASE_BUILD']
    endif

    # https://github.com/mesonbuild/meson/issues/4301
    rc_target = custom_target('windows-resource',
        command : [preprocess_command, rcdefs, '@INPUT@', '@OUTPUT@'],
        build_always_stale : true,
        build_by_default : true,
        input : 'taisei.rc.in',
        output : 'taisei.rc',
    )

    version_deps += winmod.compile_resources(
        rc_target,
        depend_files : files(
            'taisei.manifest',
            icon_main,
            icon_replay,
        )
    )

    # msvcrt is dumb and only supports up to c89.
    # with this defined, alternative implementations from mingw for e.g. the
    # printf family of functions will be used, which conform to c11.
    config.set('__USE_MINGW_ANSI_STDIO', 1)
endif

use_intel_intrin = get_option('intel_intrin') and cc.links('''
#include <immintrin.h>
__attribute__((target("sse4.2")))
int main(int argc, char **argv) {
    return _mm_crc32_u8(argc, 42);
}
''', name : 'SSE 4.2 intrinsics test')

taisei_src = files(
    'aniplayer.c',
    'boss.c',
    'cli.c',
    'color.c',
    'color.c',
    'common_tasks.c',
    'config.c',
    'coroutine.c',
    'credits.c',
    'dialog.c',
    'difficulty.c',
    'ending.c',
    'enemy.c',
    'entity.c',
    'events.c',
    'framerate.c',
    'gamepad.c',
    'global.c',
    'hashtable.c',
    'hirestime.c',
    'item.c',
    'laser.c',
    'list.c',
    'log.c',
    'main.c',
    'move.c',
    'player.c',
    'plrmodes.c',
    'progress.c',
    'projectile.c',
    'projectile_prototypes.c',
    'random.c',
    'refs.c',
    'replay.c',
    'stage.c',
    'stagedraw.c',
    'stageobjects.c',
    'stagetext.c',
    'stageutils.c',
    'taskmanager.c',
    'transition.c',
    'version.c',
    'video.c',
    'video_postprocess.c',
)

if get_option('objpools')
    taisei_src += files(
        'objectpool.c',
    )
else
    taisei_src += files(
        'objectpool_fake.c',
    )
endif

if host_machine.system() == 'nx'
    taisei_src += files(
        'arch_switch.c',
    )
endif

sse42_src = []

subdir('audio')
subdir('dialog')
subdir('eventloop')
subdir('menu')
subdir('plrmodes')
subdir('renderer')
subdir('resource')
subdir('rwops')
subdir('stages')
subdir('util')
subdir('vfs')

if use_intel_intrin
    sse42_lib = static_library(
        'taisei_sse42',
        sse42_src,
        c_args : taisei_c_args + ['-msse4.2'],
        install : false
    )
    sse42_dep = declare_dependency(link_with: sse42_lib)
    taisei_deps += sse42_dep
    config.set('TAISEI_BUILDCONF_USE_SSE42', true)
    message('SSE 4.2 intrinsics will be used')
elif get_option('intel_intrin')
    config.set('TAISEI_BUILDCONF_USE_SSE42', false)
    warning('SSE 4.2 intrinsics can not be used')
endif

configure_file(configuration : config, output : 'build_config.h')

taisei_src += [
    audio_src,
    dialog_src,
    eventloop_src,
    menu_src,
    plrmodes_src,
    renderer_src,
    resource_src,
    rwops_src,
    stages_src,
    util_src,
    vfs_src,
]

taisei_deps += [
    audio_deps,
    renderer_deps,
    util_deps,
]

if taisei_deps.contains(dep_opusfile)
    taisei_src += util_opus_src
endif

taisei_basename = (macos_app_bundle ? 'Taisei' : 'taisei')

if host_machine.system() == 'emscripten'
    if get_option('b_staticpic')
        warning('b_staticpic is not compatible with the LLVM WebAssembly backend')
    endif

    em_debug = is_debug_build
    em_link_outputs = []
    em_link_output_suffixes = ['html', 'wasm', 'js']  # first element is significant
    em_data_dir = config.get_unquoted('TAISEI_BUILDCONF_DATA_PATH')
    em_link_args = [
        '-O@0@'.format(get_option('optimization')),
        '-s', 'ALLOW_MEMORY_GROWTH=1',
        '-s', 'ENVIRONMENT=web',
        '-s', 'EXIT_RUNTIME=0',
        '-s', 'EXTRA_EXPORTED_RUNTIME_METHODS=["ccall"]',
        '-s', 'EXPORT_NAME=Taisei',
        '-s', 'FILESYSTEM=1',
        '-s', 'FORCE_FILESYSTEM=1',
        '-s', 'GL_POOL_TEMP_BUFFERS=0',
        '-s', 'LZ4=1',
        '-s', 'MODULARIZE=0',
        '-s', 'TOTAL_MEMORY=268435456',
        '-s', 'USE_WEBGL2=1',
        '-s', 'WASM=1',
        # NOTE: the _SDL_LoadWAV_RW is an LTO workaround; we don't actually care about that function.
        '-s', 'EXPORTED_FUNCTIONS=["_main", "_vfs_sync_callback", "_SDL_LoadWAV_RW"]',
        '-s', 'GL_PREINITIALIZED_CONTEXT=1',
        '-lidbfs.js',

        # Try enabling this if unpatched Freetype crashes
        # '-s', 'EMULATE_FUNCTION_POINTER_CASTS=1',
    ]

    em_link_args += subproject('koishi').get_variable('koishi_external_link_args')

    if em_debug
        # em_link_output_suffixes += ['wasm.map']
        em_link_args += [
            '--emrun',
            '--profiling',
            '-g3',
            '-s', 'ASSERTIONS=2',
            '-s', 'GL_DEBUG=1',
            '-s', 'GL_ASSERTIONS=1',
            '-s', 'GL_TRACK_ERRORS=1',
        ]
    else
        em_link_args += [
            '-O@0@'.format(get_option('optimization')),
            '-g0',
            '-s', 'ASSERTIONS=0',
            '-s', 'GL_TRACK_ERRORS=0',
        ]
    endif

<<<<<<< HEAD
=======
    if get_option('b_lto')
        em_link_args += [
            # See https://github.com/emscripten-core/emscripten/issues/8574
            '-s', 'WASM_OBJECT_FILES=0',
            '--llvm-lto', '1',
        ]
    endif

>>>>>>> Emscripten compat fixups
    foreach suffix : em_link_output_suffixes
        em_link_outputs += ['@0@.@1@'.format(taisei_basename, suffix)]
    endforeach

    taisei = executable(taisei_basename, taisei_src, version_deps,
        dependencies : taisei_deps,
        c_args : taisei_c_args,
        link_args : em_link_args,
        name_suffix : 'bc',
        install : false,
    )

    taisei_html = custom_target(em_link_outputs[0],
        # NOTE: Unfortunately we can't just put 'taisei' directly into the command array.
        # Meson then makes an invalid assumption that we are going to execute it ("use as a generator"),
        # and aborts because there's no exe wrapper in the cross file (which wouldn't make sense to have).

        command : [
            cc.cmd_array(),
            taisei.full_path(),
            emscripten_global_link_args,
            em_bundle_link_args,
            '--pre-js', em_preamble,
            '--shell-file', em_shell,
            get_option('c_args'),
            get_option('c_link_args'),
            em_link_args,

            # Emscripten bug https://github.com/emscripten-core/emscripten/issues/10066
            '-s', 'WASM_OBJECT_FILES=1',

            '-o', '@OUTPUT0@',
        ],
        build_by_default : true,
        depends : [taisei],
        output : em_link_outputs,
        install : true,
        install_dir : bindir,
    )

    bindist_deps += taisei_html
elif host_machine.system() == 'nx'
    taisei_elf_name = '@0@.elf'.format(taisei_basename)
    taisei_elf = executable(taisei_elf_name, taisei_src, version_deps,
        dependencies : taisei_deps,
        c_args : taisei_c_args,
        c_pch : 'pch/taisei_pch.h',
        install : is_debug_build,
        install_dir : bindir,
        override_options: ['strip=false'],
    )
    bindist_deps += taisei_elf

    taisei_nacp_name = '@0@.nacp'.format(taisei_basename)
    taisei_nacp = custom_target(taisei_nacp_name,
        command : [
            find_program('nacptool'),
            '--create',
            nx_app_title,
            nx_app_author,
            taisei_version_string,
            '@OUTPUT@',
        ],
        build_by_default : true,
        install : false,
        output : taisei_nacp_name,
    )

    taisei_nro_name = '@0@.nro'.format(taisei_basename)
    taisei_nro = custom_target(taisei_nro_name,
        # NOTE: Unfortunately we can't just put 'taisei_elf' directly into the command array.
        # Meson then makes an invalid assumption that we are going to execute it ("use as a generator"),
        # and aborts because there's no exe wrapper in the cross file (which wouldn't make sense to have).

        command : [
            find_program('elf2nro'),
            taisei_elf.full_path(),   # workaround for the above issue
            '@OUTPUT@',
            '--nacp=@0@'.format(taisei_nacp.full_path()),   # if we could pass the path in a standalone argument, we could have meson generate an implicit dependency here...
            '--icon=@0@'.format(nx_icon_path),
        ],
        build_by_default : true,
        depends : [taisei_elf, taisei_nacp],
        install : true,
        install_dir : bindir,
        output : taisei_nro_name,
    )
    bindist_deps += taisei_nro
else
    taisei = executable(taisei_basename, taisei_src, version_deps,
        dependencies : taisei_deps,
        c_args : taisei_c_args,
        c_pch : 'pch/taisei_pch.h',
        gui_app : not get_option('win_console'),
        install : true,
        install_dir : bindir,
    )
    bindist_deps += taisei
endif
