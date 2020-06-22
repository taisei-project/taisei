
default_backend = get_option('a_default')

if default_backend == 'sdl2mixer'
    error('Your build directory is outdated. Try running the following command:\n\n$ meson --wipe @0@ @1@ -Da_default=sdl\n'.format(meson.source_root(), meson.build_root()))
endif

if not get_option('a_@0@'.format(default_backend))
    error('Default audio backend \'@0@\' is not enabled. Enable it with -Da_@0@=true, or set a_default to something else.'.format(default_backend))
endif

audio_src = files(
    'audio.c',
    'backend.c',
)

modules = [
    'sdl',
    'null',
]

audio_deps = []
enabled_audio_backends = []
included_deps = []
needed_deps = []
a_macro = []

foreach m : modules
    if get_option('a_@0@'.format(m))
        subdir(m)
        included_deps += [m]
        enabled_audio_backends += [m]
        a_macro += ['A(@0@)'.format(m)]
        audio_src += get_variable('a_@0@_src'.format(m))
        needed_deps += get_variable('a_@0@_deps'.format(m))
        audio_deps += get_variable('a_@0@_libdeps'.format(m))
    endif
endforeach

foreach dep : needed_deps
    if not included_deps.contains(dep)
        subdir(dep)
        included_deps += [dep]
        audio_src += get_variable('a_@0@_src'.format(dep))
        audio_deps += get_variable('a_@0@_libdeps'.format(dep))
    endif
endforeach

a_macro = ' '.join(a_macro)
config.set('TAISEI_BUILDCONF_AUDIO_BACKENDS', a_macro)
config.set_quoted('TAISEI_BUILDCONF_AUDIO_DEFAULT', default_backend)
