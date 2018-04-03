
renderer_src = files(
    'api.c',
)

subdir('common')
subdir('glcommon')

modules = [
    'gl33',
    'gles20',
    'gles30',
    'null',
]

included_deps = []
needed_deps = ['common']
r_macro = []

foreach m : modules
    subdir(m)
    if get_option('r_@0@'.format(m))
        renderer_src += get_variable('r_@0@_src'.format(m))
        r_macro += ['R(@0@)'.format(m)]
        needed_deps += get_variable('r_@0@_deps'.format(m))
    endif
endforeach

foreach dep : needed_deps
    if not included_deps.contains(dep)
        included_deps += [dep]
        renderer_src += get_variable('r_@0@_src'.format(dep))
    endif
endforeach

r_macro = ' '.join(r_macro)
config.set('TAISEI_BUILDCONF_RENDERER_BACKENDS', r_macro)
config.set_quoted('TAISEI_BUILDCONF_RENDERER_DEFAULT', get_option('r_default'))
