modules = [
    'gl33',
    'gles20',
    'gles30',
    'null',
]

if host_machine.system() == 'nx'
    has_forced_renderer = true
    forced_renderer = 'gles30'
else
    has_forced_renderer = false
    forced_renderer = ''
endif

default_renderer = has_forced_renderer ? forced_renderer : get_option('r_default')

if not has_forced_renderer and not get_option('r_@0@'.format(default_renderer))
    error('Default renderer \'@0@\' is not enabled. Enable it with -Dr_@0@=true, or set r_default to something else.'.format(default_renderer))
endif

renderer_src = files(
    'api.c',
)

renderer_deps = []
enabled_renderers = []

# NOTE: Order matters here.
subdir('null')
subdir('common')
subdir('glcommon')
subdir('gl33')
subdir('glescommon')
subdir('gles20')
subdir('gles30')

included_deps = []
needed_deps = ['common']
r_macro = []

foreach m : modules
    should_include = has_forced_renderer ? m == forced_renderer : get_option('r_@0@'.format(m))
    if should_include
        renderer_src += get_variable('r_@0@_src'.format(m))
        r_macro += ['R(@0@)'.format(m)]
        enabled_renderers += [m]
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
config.set_quoted('TAISEI_BUILDCONF_RENDERER_DEFAULT', default_renderer)
