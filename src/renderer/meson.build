
default_backend = get_option('r_default')

if not get_option('r_@0@'.format(default_backend))
    error('Default renderer \'@0@\' is not enabled. Enable it with -Dr_@0@=true, or set r_default to something else.'.format(default_backend))
endif

renderer_src = files(
    'api.c',
)

renderer_deps = []

# NOTE: Order matters here.
subdir('null')
subdir('common')
subdir('glcommon')
subdir('gl33')
subdir('glescommon')
subdir('gles20')
subdir('gles30')

modules = [
    'gl33',
    'gles20',
    'gles30',
    'null',
]

included_deps = []
needed_deps = ['common']
r_macro = []

if get_option('shader_transpiler')
    subdir('spirv_tools')
    needed_deps += ['spirv_tools']
else
    subdir('spirv_tools_stub')
    needed_deps += ['spirv_tools_stub']
endif

foreach m : modules
    if get_option('r_@0@'.format(m))
        renderer_src += get_variable('r_@0@_src'.format(m))
        r_macro += ['R(@0@)'.format(m)]
        needed_deps += get_variable('r_@0@_deps'.format(m))
        included_deps += [m]
        renderer_deps += get_variable('r_@0@_libdeps'.format(m))
    endif
endforeach

foreach dep : needed_deps
    if not included_deps.contains(dep)
        included_deps += [dep]
        renderer_src += get_variable('r_@0@_src'.format(dep))
        renderer_deps += get_variable('r_@0@_libdeps'.format(dep))
    endif
endforeach

r_macro = ' '.join(r_macro)
config.set('TAISEI_BUILDCONF_RENDERER_BACKENDS', r_macro)
config.set_quoted('TAISEI_BUILDCONF_RENDERER_DEFAULT', get_option('r_default'))
