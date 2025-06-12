
memory_src = files(
    'allocator.c',
    'arena.c',
    'memory.c',
    'mempool.c',
    'scratch.c',
)

allocator = get_option('allocator')
orig_allocator = allocator

if allocator == 'auto'
    if host_machine.system() == 'windows'
        allocator = 'mimalloc'
    else
        allocator = 'libc'
    endif
endif

if allocator == 'mimalloc'
    dep_mimalloc = dependency('mimalloc',
        required : orig_allocator == 'mimalloc')
    if dep_mimalloc.found()
        taisei_deps += dep_mimalloc
    else
        allocator = 'libc'
    endif
endif

memory_src += files('backend_@0@.c'.format(allocator))
