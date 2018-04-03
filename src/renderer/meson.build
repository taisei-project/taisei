
renderer_src = files(
    'api.c',
)

subdir('common')
renderer_src += r_common_src

modules = [
    'gl33',
    'null',
]

r_macro = []

foreach m : modules
    if get_option('r_@0@'.format(m))
        subdir(m)
        renderer_src += get_variable('r_@0@_src'.format(m))
        r_macro += ['R(@0@)'.format(m)]
    endif
endforeach

r_macro = ' '.join(r_macro)
config.set('TAISEI_BUILDCONF_RENDERER_BACKENDS', r_macro)
config.set_quoted('TAISEI_BUILDCONF_RENDERER_DEFAULT', get_option('r_default'))
