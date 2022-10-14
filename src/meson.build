
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
    'dynarray.c',
    'enemy.c',
    'enemy_classes.c',
    'entity.c',
    'events.c',
    'framerate.c',
    'gamepad.c',
    'global.c',
    'hashtable.c',
    'hirestime.c',
    'item.c',
    'list.c',
    'log.c',
    'main.c',
    'move.c',
    'player.c',
    'plrmodes.c',
    'portrait.c',
    'progress.c',
    'projectile.c',
    'projectile_prototypes.c',
    'random.c',
    'refs.c',
    'stage.c',
    'stagedraw.c',
    'stageinfo.c',
    'stageobjects.c',
    'stagetext.c',
    'stageutils.c',
    'stats.c',
    'taskmanager.c',
    'transition.c',
    'version.c',
    'video.c',
    'video_postprocess.c',
)

if is_developer_build
    taisei_src += files(
        'camcontrol.c'
    )
endif

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

subdir('audio')
subdir('cutscenes')
subdir('dialog')
subdir('eventloop')
subdir('filewatch')
subdir('lasers')
subdir('menu')
subdir('pixmap')
subdir('plrmodes')
subdir('renderer')
subdir('replay')
subdir('resource')
subdir('rwops')
subdir('stages')
subdir('util')
subdir('vfs')

taisei_src += [
    audio_src,
    cutscenes_src,
    dialog_src,
    eventloop_src,
    filewatch_src,
    lasers_src,
    menu_src,
    pixmap_src,
    plrmodes_src,
    renderer_src,
    replay_src,
    resource_src,
    rwops_src,
    util_src,
    vfs_src,
]

taisei_deps += [
    audio_deps,
    renderer_deps,
    util_deps,
]

if stages_live_reload
    taisei_src += files('dynstage.c')

    # We want this to build fast
    libstages_opts = ['b_lto=false']
    if get_option('optimization') != '0'
        # Don't force a higher optimization level if we're building the project
        # without optimizations
        libstages_opts += ['optimization=g']
    endif

    taisei_stages = shared_module('taisei-stages', stages_src, version_deps,
        dependencies : taisei_deps,
        c_args : taisei_c_args,
        c_pch : 'pch/taisei_pch.h',
        build_by_default : true,
        gnu_symbol_visibility : 'hidden',
        override_options : libstages_opts,
    )

    taisei_deps += cc.find_library('dl', required : false)

    taisei_stages_compile_command = [
        # FIXME: no good way to get the effective meson command…
        'meson', 'compile', '-C', meson.project_build_root(), '-v', taisei_stages.name(),
    ]

    r = run_command(format_array_command, taisei_stages_compile_command, check : true)

    config.set_quoted('TAISEI_BUILDCONF_DYNSTAGE_LIB', taisei_stages.full_path())
    config.set_quoted('TAISEI_BUILDCONF_DYNSTAGE_SRCROOT', stages_srcdir)
    config.set('TAISEI_BUILDCONF_DYNSTAGE_REBUILD_CMD', r.stdout().strip())
else
    taisei_src += files('dynstage_stub.c')
    taisei_src += stages_src
endif

configure_file(configuration : config, output : 'build_config.h')

taisei_basename = (macos_app_bundle ? 'Taisei' : 'taisei')

if host_machine.system() == 'emscripten'
    if get_option('b_staticpic')
        warning('b_staticpic is not compatible with the LLVM WebAssembly backend')
    endif

    # Asyncify instrumentation is very overzealous by default.
    # This crazy list is not exhaustive, but still helps us reduce code size quite a bit.
    # Link with -s ASYNCIFY_ADVISE to see what gets instrumented.
    asyncify_ignore = [
        '*printf',
        'AlphaApplyFilter',
        'ApplyInverseTransforms',
        'Bezier_*',
        'DecodeImageData',
        'Emscripten_*',
        'FT_*',
        'FlushRenderCommands',
        'HIDAPI_*',
        'SDL_*',
        'T1_*',
        'TT_*',
        'VP8*',
        'WebP*',
        'ZSTD_*',
        '__*',
        '_audio_*',
        '_dynarray_*',
        '_free_resources',
        '_ht_*',
        '_r_*',
        '_text_draw',
        'af_*',
        'afm_*',
        'apply_kerning',
        'astream_*',
        'audio_*',
        'basist*',
        'bgm_*',
        'bind_isactive',
        'bind_setvalue',
        'calloc',
        'camera3d_*',
        'cf2_*',
        'cff_*',
        'deflate*',
        'draw_menu_list',
        'draw_transition',
        'ent_draw',
        'fbmgr_*',
        'fclose',
        'fread',
        'free',
        'ft_*',
        'gamepad_*',
        'get_exts',
        'get_glyph',
        'gl33_*',
        'glcommon_*',
        'gles30_*',
        'hash_bucket',
        'ht_*',
        'inflate*',
        'list_foreach',
        'load_resource',
        'load_resource_finish',
        'log_*',
        'malloc',
        'op_get_next_page',
        'op_seek_helper',
        'parse_keyvalue_*',
        'pfr_*',
        'pixmap_*',
        'play_sfx',
        'play_sfx_*',
        'player_property',
        'png_*',
        'preload_path',
        'printf_*',
        'px_*',
        'qsort',
        'r_*',
        'replace_sfx',
        'resource_for_each',
        'rwutil_*',
        'sfnt_*',
        'sift',
        'stage3d_*',
        'stop_sfx',
        'stop_sfx_fadeout',
        'strtox',
        'submit_play_sfx',
        't1_*',
        't42_*',
        'taisei_commit_persistent_data',
        'task_detach',
        'task_finish',
        'task_free',
        'taskmgr_*',
        'tt_*',
        'vfs_decref',
        'vfs_incref',
        'vfs_node_*',
        'video_*',
    ]

    em_debug = is_debug_build
    em_link_outputs = []
    em_link_output_suffixes = ['html', 'wasm', 'js']  # first element is significant
    em_data_dir = config.get_unquoted('TAISEI_BUILDCONF_DATA_PATH')
    em_common_args = [
        '-s', 'DEFAULT_TO_CXX=0',
    ]
    em_link_args = [
        '-O@0@'.format(get_option('optimization')),
        '-s', 'ALLOW_MEMORY_GROWTH=1',
        '-s', 'ASYNCIFY_REMOVE=@0@'.format(','.join(asyncify_ignore)),
        '-s', 'AUTO_ARCHIVE_INDEXES=0',
        '-s', 'AUTO_JS_LIBRARIES=0',
        '-s', 'AUTO_NATIVE_LIBRARIES=0',
        '-s', 'DEFAULT_LIBRARY_FUNCS_TO_INCLUDE=["$autoResumeAudioContext"]',
        '-s', 'DYNAMIC_EXECUTION=0',
        '-s', 'ENVIRONMENT=web',
        '-s', 'EXIT_RUNTIME=0',
        '-s', 'EXPORTED_FUNCTIONS=["_main", "_vfs_sync_callback"]',
        '-s', 'EXPORTED_RUNTIME_METHODS=["ccall"]',
        '-s', 'FILESYSTEM=1',
        '-s', 'FORCE_FILESYSTEM=1',
        '-s', 'GL_POOL_TEMP_BUFFERS=0',
        '-s', 'GL_PREINITIALIZED_CONTEXT=1',
        '-s', 'GL_SUPPORT_AUTOMATIC_ENABLE_EXTENSIONS=0',
        '-s', 'GL_SUPPORT_SIMPLE_ENABLE_EXTENSIONS=0',
        '-s', 'IGNORE_MISSING_MAIN=0',
        '-s', 'INITIAL_MEMORY=268435456',
        '-s', 'LLD_REPORT_UNDEFINED',
        '-s', 'LZ4=1',
        '-s', 'MAX_WEBGL_VERSION=2',
        '-s', 'MIN_WEBGL_VERSION=2',
        '-s', 'MODULARIZE=0',
        '-s', 'STRICT_JS=1',
        '-s', 'SUPPORT_BIG_ENDIAN=1',
        '-s', 'WASM=1',
        '-lGL',
        '-legl.js',
        '-lidbfs.js',
        '-lwebgl.js',
    ]

    em_link_args += subproject('koishi').get_variable('koishi_external_link_args')

    if em_debug
        em_link_output_suffixes += ['wasm.map']
        em_link_args += [
            '--emrun',
            '--profiling',
            '-g3',
            '-gsource-map',
            '--source-map-base', meson.get_cross_property('source_map_base', 'http://0.0.0.0:6931/'),
            '-s', 'ASSERTIONS=2',
            '-s', 'GL_ASSERTIONS=1',
            '-s', 'GL_DEBUG=1',
            '-s', 'GL_TRACK_ERRORS=1',
        ]
    else
        em_link_args += [
            '-O@0@'.format(get_option('optimization')),
            '-g0',
            '-s', 'ASSERTIONS=0',
            '-s', 'GL_TRACK_ERRORS=0',
        ]

        if get_option('optimization') != '0'
            em_link_args += ['--closure', '1']
        endif
    endif

    foreach suffix : em_link_output_suffixes
        em_link_outputs += ['@0@.@1@'.format(taisei_basename, suffix)]
    endforeach

    libtaisei = static_library(taisei_basename, taisei_src, version_deps,
        dependencies : taisei_deps,
        c_pch : 'pch/taisei_pch.h',
        c_args : [em_common_args, taisei_c_args],
        install : false,
    )

    taisei = static_library(taisei_basename + '-full',
        link_whole : libtaisei,
    )

    taisei_html = custom_target(em_link_outputs[0],
        command : [
            meson.get_compiler('cpp').cmd_array(),
            taisei,
            '--pre-js', em_preamble,
            em_bundle_link_args,
            '--shell-file', em_shell,
            get_option('c_args'),
            get_option('c_link_args'),
            em_common_args,
            em_link_args,
            '-o', '@OUTPUT0@',
        ],
        build_by_default : true,
        output : em_link_outputs,
        install : true,
        install_dir : bindir,
        console : true,
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
        export_dynamic : stages_live_reload,
    )
    bindist_deps += taisei
endif
