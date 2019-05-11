
a_sdl2mixer_src = files(
    'audio_sdl2mixer.c',
)

a_sdl2mixer_deps = []
a_sdl2mixer_libdeps = [dep_sdl2_mixer, dep_opusfile]

if get_option('a_sdl2mixer')
    assert(dep_sdl2_mixer.found(), 'sdl2mixer backend enabled, but SDL2_mixer was not found. Install SDL2_mixer or disable a_sdl2mixer')
    assert(dep_opusfile.found(), 'sdl2mixer backend enabled, but opusfile was not found. Install opusfile or disable a_sdl2mixer')
endif
