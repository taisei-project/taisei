project('mimalloc-cmake-wrapper')

cmake = import('cmake')
mimalloc_options = cmake.subproject_options()
mimalloc_options.set_install(false)
mimalloc_options.add_cmake_defines({
    'MI_BUILD_SHARED': false,
    'MI_BUILD_STATIC': true,
    'MI_BUILD_OBJECT': false,
    'MI_BUILD_TESTS': false,
})

# TODO detect musl and set MI_LIBC_MUSL?

passthrough_toggles = [
    'secure',
    'debug_full',
    'padding',
    'override',
    'xmalloc',
    'show_errors',
    'track_valgrind',
    'track_asan',
    'track_etw',
    'local_dynamic_tls',
    'libc_musl',
    'guarded',
    'no_padding',
    'no_thp',
]

foreach toggle : passthrough_toggles
    opt = get_option(toggle)

    if not opt.auto()
        mimalloc_options.add_cmake_defines({ 'MI_@0@'.format(toggle.to_upper()) : opt.enabled() })
    endif
endforeach

mimalloc_subproject = cmake.subproject('mimalloc', options : mimalloc_options)
mimalloc_dep = mimalloc_subproject.dependency('mimalloc-static')
