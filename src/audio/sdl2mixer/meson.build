
a_sdl2mixer_src = files(
    'audio_sdl2mixer.c',
)

a_sdl2mixer_deps = []
a_sdl2mixer_libdeps = [dep_sdl2_mixer]

if get_option('a_sdl2mixer') and not dep_sdl2_mixer.found()
    error('sdl2mixer backend enabled, but SDL2_mixer was not found. Install SDL2_mixer or disable a_sdl2mixer')
endif
