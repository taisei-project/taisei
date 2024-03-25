project('sdl3-cmake-wrapper')

cmake = import('cmake')
sdl3_options = cmake.subproject_options()
sdl3_options.set_install(false)
sdl3_options.add_cmake_defines({
    'SDL_DISABLE_INSTALL': true,
    'SDL_DISABLE_UNINSTALL': true,
    'SDL_RENDER': false,
    'SDL_POWER': false,
    'SDL_HAPTIC': false,
    'SDL_SENSOR': false,
    'SDL_EXAMPLES': false,
    'SDL_TESTS': false,
    'SDL_SHARED': false,
    'SDL_STATIC': true,
})

passthrough_toggles = [
    'alsa',
    'pipewire',
    'pthreads',
    'pulseaudio',
    'system_iconv',
    'wayland',
    'x11',
]

foreach toggle : passthrough_toggles
    opt = get_option(toggle)

    if not opt.auto()
        sdl3_options.add_cmake_defines({ 'SDL_@0@'.format(toggle.to_upper()) : opt.enabled() })
    endif
endforeach

sdl3_subproject = cmake.subproject('sdl3', options : sdl3_options)
sdl3_dep = sdl3_subproject.dependency('SDL3-static')
