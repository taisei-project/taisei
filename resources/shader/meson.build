
glsl_files = files()

glsl_files += files(
# @begin glsl
    'alpha_depth.frag.glsl',
    'bloom.frag.glsl',
    'boss_zoom.frag.glsl',
    'circleclipped_indicator.frag.glsl',
    'glitch.frag.glsl',
    'graph.frag.glsl',
    'hud_text.frag.glsl',
    'ingame_menu.frag.glsl',
    'laser_generic.vert.glsl',
    'marisa_laser.frag.glsl',
    'maristar_bombbg.frag.glsl',
    'masterspark.frag.glsl',
    'negative.frag.glsl',
    'spellcard_intro.frag.glsl',
    'spellcard_outro.frag.glsl',
    'spellcard_walloftext.frag.glsl',
    'sprite_bullet.frag.glsl',
    'sprite_bullet_apple.frag.glsl',
    'sprite_default.frag.glsl',
    'sprite_default.vert.glsl',
    'sprite_silhouette.frag.glsl',
    'sprite_silhouette.vert.glsl',
    'sprite_youmu_charged_shot.frag.glsl',
    'sprite_youmu_charged_shot.vert.glsl',
    'sprite_youmu_myon_shot.frag.glsl',
    'stage6_sky.frag.glsl',
    'stage6_sky.vert.glsl',
    'stagetext.frag.glsl',
    'standard.frag.glsl',
    'standard.vert.glsl',
    'standardnotex.frag.glsl',
    'standardnotex.vert.glsl',
    'texture_post_load.frag.glsl',
    'tower_light.frag.glsl',
    'tower_light.vert.glsl',
    'tower_wall.frag.glsl',
    'tower_wall.vert.glsl',
    'tunnel.frag.glsl',
    'youmua_bomb.frag.glsl',
    'zbuf_fog.frag.glsl',
# @end glsl
)

subdirs = [
# @begin subdirs
    'lasers',
# @end subdirs
]

foreach sd : subdirs
    subdir(sd)
endforeach

glsl_targets = []

if get_option('validate_glsl') != 'false'
    validate_glsl = false

    glslc_command = find_program('glslc',
        required : (get_option('validate_glsl') == 'true')
    )

    if glslc_command.found()
        test_result = run_command(glslc_command,
            # '-fauto-bind-uniforms',
            # '-fauto-map-locations',
            '--target-env=opengl',
            '-fshader-stage=frag',
            '-DFRAG_STAGE',
            # '-Werror',
            glslc_test_file,
            '-o',
            join_paths(meson.current_build_dir(), 'glslc-test.spv')
        )

        if test_result.returncode() == 0
            validate_glsl = true
        else
            message(test_result.stderr())

            if get_option('validate_glsl') == 'auto'
                message('glslc test failed, you probably have an incompatible version. GLSL validation will be disabled.')
            else
                error('glslc test failed, you probably have an incompatible version.')
            endif
        endif
    endif

    if validate_glsl
        foreach src : glsl_files
            fname = '@0@'.format(src)
            stage = fname.split('.')[-2]

            glsl_targets += custom_target(fname.underscorify(),
                input : src,
                output : '@BASENAME@.spv',
                command : [glslc_command,
                    # '-fauto-bind-uniforms',
                    # '-fauto-map-locations',

                    # XXX: glslc currently doesn't support ARB_explicit_uniform_location, so we have
                    # to force a standard that supports it natively to check for location overlaps.
                    '-std=430core',

                    # TODO: somehow shut up that annoying warning caused by forcing glsl version.
                    # '-Werror',

                    '--target-env=opengl',
                    '-fshader-stage=@0@'.format(stage),
                    '-D@0@_STAGE'.format(stage.to_upper()),
                    '-MD',
                    '-MF',
                    '@DEPFILE@',
                    '@INPUT@',
                    '-o',
                    '@OUTPUT@'
                ],
                install : false,
                build_always : false,
                build_by_default : true,
                depfile : '@0@.d'.format(fname.underscorify()),
            )
        endforeach
    endif
endif

