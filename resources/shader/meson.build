
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
    'recolor.frag.glsl',
    'spellcard_intro.frag.glsl',
    'spellcard_outro.frag.glsl',
    'spellcard_walloftext.frag.glsl',
    'sprite_bullet.frag.glsl',
    'sprite_bullet_apple.frag.glsl',
    'sprite_common.frag.glsl',
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
    glslc_command = find_program('glslc',
        required : (get_option('validate_glsl') == 'true')
    )

    if glslc_command.found()
        foreach src : glsl_files
            fname = '@0@'.format(src)
            stage = fname.split('.')[-2]

            glsl_targets += custom_target(fname.underscorify(),
                input : src,
                output : '@BASENAME@.spv',
                command : [glslc_command,
                    '-fauto-bind-uniforms',
                    '-fauto-map-locations',
                    '--target-env=opengl',
                    '-fshader-stage=@0@'.format(stage),
                    '-Werror',
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

