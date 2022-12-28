
audio_backend_opts = {
    'sdl' : get_option('a_sdl'),
    'null' : get_option('a_null'),
}

default_audio_backend = get_option('a_default')
audio_backends_prio = ['sdl', 'null']

if default_audio_backend == 'auto'
    foreach audio_backend : audio_backends_prio
        if audio_backend_opts[audio_backend].allowed()
            default_audio_backend = audio_backend
            break
        endif
    endforeach

    if default_audio_backend == 'auto'
        audio_backends_prio_opts = []
        foreach audio_backend : audio_backends_prio
            audio_backends_prio_opts + f'a_@audio_backend@'
        endforeach
        audio_backends_prio_opts = ', '.join(audio_backends_prio_opts)

        error('Could not pick a default audio backend. ' +
             f'Make sure you have at least one of audio_backends_prio_opts enabled.')
    endif
endif

assert(audio_backend_opts[default_audio_backend].allowed(),
    f'Default audio backend @default_audio_backend@ is not enabled')

audio_src = files(
    'audio.c',
    'backend.c',
)

audio_deps = []
enabled_audio_backends = []
included_deps = []
needed_deps = []
a_macro = []

foreach audio_backend, aopt : audio_backend_opts
    if aopt.allowed()
        subdir(audio_backend)
        included_deps += [audio_backend]
        enabled_audio_backends += [audio_backend]
        a_macro += ['A(@0@)'.format(audio_backend)]
        audio_src += get_variable(f'a_@audio_backend@_src')
        needed_deps += get_variable(f'a_@audio_backend@_deps')
        audio_deps += get_variable(f'a_@audio_backend@_libdeps')
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
config.set_quoted('TAISEI_BUILDCONF_AUDIO_DEFAULT', default_audio_backend)
