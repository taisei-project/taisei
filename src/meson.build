ver_fb = get_option('version_fallback').strip()
version_fallback = ver_fb != '' ? ver_fb : meson.project_version()

version_script = find_program('../scripts/version.py')
version_command = [version_script, meson.source_root(), '@INPUT@', '@OUTPUT@', get_option('buildtype'), version_fallback]
version_deps = [custom_target('version file',
                              command : version_command,
                              build_always : true,
                              input : 'version_auto.c.in',
                              output : 'version_auto.c')]

if host_machine.system() == 'windows'
    winmod = import('windows')
    # HACK: compile_resources only accepts strings, not generated dependencies.
    # We still want to generate this every time though.
    rcpath = join_paths(meson.current_build_dir(), 'taisei.rc')
    c = run_command(version_script, meson.source_root(), 'taisei.rc.in', rcpath, get_option('buildtype'), version_fallback)
    if c.returncode() != 0
        error('Failed to generate Windows resource file:\n' + c.stderr())
    endif
    version_deps += custom_target('Windows resource file',
                                  command : version_command,
                                  build_always : true,
                                  input : 'taisei.rc.in',
                                  output : 'taisei.rc')
    version_deps += winmod.compile_resources(rcpath)
endif

configure_file(configuration : config, output : 'build_config.h')

src_base = [
    'main.c',
    'cli.c',
    'log.c',
    'util.c',
    'taiseigl.c',
    'random.c',
    'config.c',
    'color.c',
    'recolor.c',
    'gamepad.c',
    'stage.c',
    'stagedraw.c',
    'stagetext.c',
    'replay.c',
    'global.c',
    'events.c',
    'player.c',
    'aniplayer.c',
    'projectile.c',
    'progress.c',
    'enemy.c',
    'item.c',
    'list.c',
    'hashtable.c',
    'boss.c',
    'plrmodes.c',
    'plrmodes/marisa.c',
    'plrmodes/marisa_a.c',
    'plrmodes/marisa_b.c',
    'plrmodes/youmu.c',
    'plrmodes/youmu_a.c',
    'plrmodes/youmu_b.c',
    'laser.c',
    'dialog.c',
    'fbo.c',
    'vbo.c',
    'stageutils.c',
    'matrix.c',
    'video.c',
    'transition.c',
    'color.c',
    'difficulty.c',
    'audio_common.c',
    'hirestime.c',
    'menu/menu.c',
    'menu/mainmenu.c',
    'menu/options.c',
    'menu/stageselect.c',
    'menu/replayview.c',
    'menu/ingamemenu.c',
    'menu/gameovermenu.c',
    'menu/savereplay.c',
    'menu/difficultyselect.c',
    'menu/charselect.c',
    'menu/stagepractice.c',
    'menu/submenus.c',
    'menu/spellpractice.c',
    'menu/common.c',
    'stages/stage1.c',
    'stages/stage1_events.c',
    'stages/stage2.c',
    'stages/stage2_events.c',
    'stages/stage3.c',
    'stages/stage3_events.c',
    'stages/stage4.c',
    'stages/stage4_events.c',
    'stages/stage5.c',
    'stages/stage5_events.c',
    'stages/stage6.c',
    'stages/stage6_events.c',
    'ending.c',
    'credits.c',
    'resource/resource.c',
    'resource/texture.c',
    'resource/animation.c',
    'resource/font.c',
    'resource/shader.c',
    'resource/model.c',
    'resource/postprocess.c',
    'rwops/rwops_dummy.c',
    'rwops/rwops_zlib.c',
    'rwops/rwops_segment.c',
    'rwops/rwops_autobuf.c',
    'vfs/private.c',
    'vfs/public.c',
    'vfs/setup.c',
    'vfs/pathutil.c',
    'vfs/union.c',
    'vfs/union_public.c',
    'vfs/vdir.c',
    'vfs/syspath_public.c',
    'vfs/zipfile_public.c',
    'version.c',
]

if taisei_deps.contains(dep_sdl2_mixer)
    src_base += [
        'audio_mixer.c',
        'resource/bgm_mixer.c',
        'resource/sfx_mixer.c',
    ]
else
    src_base += [
        'audio_null.c',
        'resource/bgm_null.c',
        'resource/sfx_null.c',
    ]
endif

if taisei_deps.contains(dep_zip)
    src_base += [
        'rwops/rwops_zipfile.c',
        'vfs/zipfile.c',
        'vfs/zippath.c',
    ]
else
    src_base += ['vfs/zipfile_null.c']
endif

if have_posix
    src_base += [
        'vfs/syspath_posix.c',
        'rwops/rwops_pipe_posix.c',
    ]
elif host_machine.system() == 'windows'
    src_base += [
        'windows_misc.c',
        'vfs/syspath_win32.c',
        'rwops/rwops_pipe_null.c',
    ]
else
    src_base += [
        'vfs/syspath_posix.c',      # eeehh, maybe it'll work ¯\_(ツ)_/¯
        'rwops/rwops_pipe_null.c',
    ]
endif

taisei_exe = executable('taisei', src_base, version_deps,
                        dependencies : taisei_deps,
                        c_args : taisei_c_args,
                        gui_app : not get_option('win_console'),
                        install : true)
