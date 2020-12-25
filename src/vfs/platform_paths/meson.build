
vfs_platform_paths_src = []

if is_relocatable_install
    vfs_platform_paths_src += files('resroot_sdl.c')
else
    vfs_platform_paths_src += files('resroot_static.c')
endif

vfs_platform_paths_src += files('storage_sdl.c')

xdg_systems = [
    'linux',
    'freebsd',
    'openbsd',
    'netbsd',
    'dragonfly',
    'cygwin',
]

if host_machine.system() == 'windows'
    vfs_platform_paths_src += files('cache_win32.c')
elif xdg_systems.contains(host_machine.system())
    vfs_platform_paths_src += files('cache_xdg.c')
elif host_machine.system() == 'darwin'
    add_languages('objc')
    taisei_deps += dependency('appleframeworks', modules : 'foundation')
    vfs_platform_paths_src += files('cache_apple.m')
else
    vfs_platform_paths_src += files('cache_generic.c')
endif
