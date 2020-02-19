modules = [
    'gl33',
    'gles20',
    'gles30',
    'null',
]

if ['nx', 'emscripten'].contains(host_machine.system())
    forced_renderer = 'gles30'
else
    forced_renderer = ''
endif

default_renderer = (forced_renderer == '') ? get_option('r_default') : forced_renderer
static_gles30 = (host_machine.system() == 'emscripten')

if forced_renderer == '' and not get_option('r_@0@'.format(default_renderer))
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
    should_include = (forced_renderer == '') ? get_option('r_@0@'.format(m)) : (m == forced_renderer)
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
config.set('TAISEI_BUILDCONF_HAVE_ANGLE',
    enabled_renderers.contains('gles30') and
    get_option('install_angle')
)
