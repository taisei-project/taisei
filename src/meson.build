
version_deps = [
    preprocessor.process('version_auto.c.in')
]

if host_machine.system() == 'windows'
    winmod = import('windows')

    # HACK: compile_resources only accepts strings, not generated dependencies.
    # We still want to generate this every time though.

    # This is unreliable.
    # https://github.com/mesonbuild/meson/issues/2807

    rcpath = join_paths(meson.current_build_dir(), 'taisei.rc')
    rcdefs = [
        '-DICONS_DIR=@0@'.format(join_paths(meson.source_root(), 'misc', 'icons'))
    ]

    if get_option('buildtype').startswith('debug')
        rcdefs += ['-DBUILDTYPE_DEFINE=#define DEBUG_BUILD']
    else
        rcdefs += ['-DBUILDTYPE_DEFINE=#define RELEASE_BUILD']
    endif

    c = run_command(preprocess_command, 'taisei.rc.in', rcpath, rcdefs)

    if c.returncode() != 0
        error('Failed to generate Windows resource file:\n' + c.stderr())
    endif

    rc_target = custom_target('Windows resource file',
        command : [preprocess_command, rcdefs, '@INPUT@', '@OUTPUT@'],
        build_always : true,
        input : 'taisei.rc.in',
        output : 'taisei.rc',
    )

    version_deps += rc_target
    version_deps += winmod.compile_resources(rc_target.full_path())

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

if use_intel_intrin
    # All that just to append that -msse4.2 argument for this one file.
    sse42_lib = static_library('taisei_sse42', 'util_sse42.c', c_args: taisei_c_args + ['-msse4.2'], install : false)
    sse42_dep = declare_dependency(link_with: sse42_lib)
    taisei_deps += sse42_dep
    message('SSE 4.2 intrinsics will be used')
elif get_option('intel_intrin')
    message('SSE 4.2 intrinsics can not be used')
endif

configure_file(configuration : config, output : 'build_config.h')

src_base = [
    'aniplayer.c',
    'audio_common.c',
    'boss.c',
    'cli.c',
    'color.c',
    'color.c',
    'config.c',
    'credits.c',
    'dialog.c',
    'difficulty.c',
    'ending.c',
    'enemy.c',
    'events.c',
    'fbo.c',
    'gamepad.c',
    'global.c',
    'hashtable.c',
    'hirestime.c',
    'item.c',
    'laser.c',
    'list.c',
    'log.c',
    'main.c',
    'matrix.c',
    'menu/charselect.c',
    'menu/common.c',
    'menu/difficultyselect.c',
    'menu/gameovermenu.c',
    'menu/ingamemenu.c',
    'menu/mainmenu.c',
    'menu/menu.c',
    'menu/options.c',
    'menu/replayview.c',
    'menu/savereplay.c',
    'menu/spellpractice.c',
    'menu/stagepractice.c',
    'menu/stageselect.c',
    'menu/submenus.c',
    'objectpool.c',
    'objectpool_util.c',
    'player.c',
    'plrmodes.c',
    'plrmodes/marisa_a.c',
    'plrmodes/marisa_b.c',
    'plrmodes/marisa.c',
    'plrmodes/youmu_a.c',
    'plrmodes/youmu_b.c',
    'plrmodes/youmu.c',
    'plrmodes/reimu_a.c',
    'plrmodes/reimu_b.c',
    'plrmodes/reimu.c',
    'progress.c',
    'projectile.c',
    'random.c',
    'recolor.c',
    'refs.c',
    'replay.c',
    'resource/animation.c',
    'resource/font.c',
    'resource/model.c',
    'resource/postprocess.c',
    'resource/resource.c',
    'resource/shader.c',
    'resource/texture.c',
    'rwops/rwops_autobuf.c',
    'rwops/rwops_dummy.c',
    'rwops/rwops_segment.c',
    'rwops/rwops_zlib.c',
    'stage.c',
    'stagedraw.c',
    'stageobjects.c',
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
    'stagetext.c',
    'stageutils.c',
    'taiseigl.c',
    'transition.c',
    'util.c',
    'vbo.c',
    'version.c',
    'vfs/pathutil.c',
    'vfs/private.c',
    'vfs/public.c',
    'vfs/setup.c',
    'vfs/syspath_public.c',
    'vfs/union.c',
    'vfs/union_public.c',
    'vfs/vdir.c',
    'vfs/zipfile_public.c',
    'video.c',
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
    src_base += [
        'vfs/zipfile_null.c',
    ]
endif

if have_posix
    src_base += [
        'vfs/syspath_posix.c',
        'rwops/rwops_pipe_posix.c',
    ]
elif host_machine.system() == 'windows'
    src_base += [
        # 'windows_misc.c',
        'vfs/syspath_win32.c',
        'rwops/rwops_pipe_null.c',
    ]
else
    src_base += [
        'vfs/syspath_posix.c',      # eeehh, maybe it'll work ¯\_(ツ)_/¯
        'rwops/rwops_pipe_null.c',
    ]
endif

if macos_app_bundle
    taisei_exe_name = 'Taisei'
else
    taisei_exe_name = 'taisei'
endif

taisei_exe = executable(taisei_exe_name, src_base, version_deps,
    dependencies : taisei_deps,
    c_args : taisei_c_args,
    gui_app : not get_option('win_console'),
    install : true,
    install_dir : bindir,
)
