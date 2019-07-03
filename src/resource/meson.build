
resource_src = files(
    'animation.c',
    'bgm.c',
    'bgm_metadata.c',
    'font.c',
    'model.c',
    'postprocess.c',
    'resource.c',
    'sfx.c',
    'sfxbgm_common.c',
    'shader_object.c',
    'shader_program.c',
    'sprite.c',
    'texture.c',
)

sizeof_ptr = cc.sizeof('void*')

config.set('TAISEI_BUILDCONF_SIZEOF_PTR', sizeof_ptr)
config.set('TAISEI_BUILDCONF_RESREF_TYPE_PORTABLE', '0')
config.set('TAISEI_BUILDCONF_RESREF_TYPE_X64', '1')
config.set('TAISEI_BUILDCONF_RESREF_TYPE_AARCH64', '2')

if sizeof_ptr == 8
    if host_machine.cpu_family() == 'x86_64'
        config.set('TAISEI_BUILDCONF_RESREF_TYPE', 'TAISEI_BUILDCONF_RESREF_TYPE_X64')
    elif host_machine.cpu_family() == 'aarch64'
        config.set('TAISEI_BUILDCONF_RESREF_TYPE', 'TAISEI_BUILDCONF_RESREF_TYPE_AARCH64')
    else
        config.set('TAISEI_BUILDCONF_RESREF_TYPE', 'TAISEI_BUILDCONF_RESREF_TYPE_PORTABLE')
    endif
else
    config.set('TAISEI_BUILDCONF_RESREF_TYPE', 'TAISEI_BUILDCONF_RESREF_TYPE_PORTABLE')
endif
