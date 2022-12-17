
subdir('common')

renderer_opts = {
    'gl33' : get_option('r_gl33').require(
        host_machine.system() not in ['nx', 'emscripten'],
        error_message : 'OpenGL 3.3 is not supported on this platform'),
    'gles20' : get_option('r_gles20').disable_auto_if(not shader_transpiler_enabled),
    'gles30' : get_option('r_gles30').disable_auto_if(
        not (shader_transpiler_enabled or transpile_glsl)),
    'null' : get_option('r_null'),
}

default_renderer = get_option('r_default')
renderers_prio = ['gl33', 'gles30', 'gles20']

if default_renderer == 'auto'
    foreach renderer : renderers_prio
        if renderer_opts[renderer].allowed()
            default_renderer = renderer
            break
        endif
    endforeach

    if default_renderer == 'auto'
        renderers_prio_opts = []
        foreach renderer : renderers_prio
            renderers_prio_opts + f'r_@renderer@'
        endforeach
        renderers_prio_opts = ', '.join(renderers_prio_opts)

        error('Could not pick a default renderer. ' +
             f'Make sure you have at least one of @renderers_prio_opts@ enabled, ' +
              'or force r_default=null if you really mean it.')
    endif
endif

assert(renderer_opts[default_renderer].allowed(),
    f'Default renderer @default_renderer@ is not enabled')

modules = [
    'gl33',
    'gles20',
    'gles30',
    'null',
]

static_gles30 = (
    renderer_opts['gles30'].allowed() and
    host_machine.system() == 'emscripten'
)

renderer_src = files(
    'api.c',
)

renderer_deps = []
enabled_renderers = []

# NOTE: Order matters here.
subdir('null')
subdir('glcommon')
subdir('gl33')
subdir('glescommon')
subdir('gles20')
subdir('gles30')

included_deps = []
needed_deps = ['common']
r_macro = []

foreach renderer, ropt : renderer_opts
    if ropt.allowed()
        renderer_src += get_variable(f'r_@renderer@_src')
        r_macro += f'R(@renderer@)'
        enabled_renderers += renderer
        needed_deps += get_variable(f'r_@renderer@_deps')
        included_deps += renderer
        renderer_deps += get_variable(f'r_@renderer@_libdeps')
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

have_gles_renderer = (
    renderer_opts['gles30'].allowed() or
    renderer_opts['gles20'].allowed())

if angle_enabled and not have_gles_renderer
    error('An OpenGL ES renderer is required to use ANGLE. ' +
          'Enable r_gles30 or r_gles20, or disable install_angle.')
endif
