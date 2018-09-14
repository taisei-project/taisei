
resource_src = files(
    'animation.c',
    'bgm.c',
    'font.c',
    'model.c',
    'postprocess.c',
    'resource.c',
    'sfx.c',
    'shader_object.c',
    'shader_program.c',
    'sprite.c',
    'texture.c',
)

if taisei_deps.contains(dep_sdl2_mixer)
    resource_src += files(
        'bgm_mixer.c',
        'sfx_mixer.c',
    )
else
    resource_src += files(
        'bgm_null.c',
        'sfx_null.c',
    )
endif
