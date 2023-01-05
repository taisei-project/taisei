
vfs_src = files(
    'decompress_wrapper.c',
    'loadpacks.c',
    'nodeapi.c',
    'pathutil.c',
    'private.c',
    'public.c',
    'readonly_wrapper.c',
    'syspath_public.c',
    'union.c',
    'vdir.c',
    'zipfile_public.c',
)

if dep_zip.found()
    vfs_src += files(
        'zipfile.c',
        'zippath.c',
        'zippath_rwops.c',
    )
else
    vfs_src += files(
        'zipfile_null.c',
    )
endif

if have_posix
    vfs_src += files('syspath_posix.c')
elif host_machine.system() == 'windows'
    vfs_src += files('syspath_win32.c')
else
    vfs_src += files('syspath_posix.c') # eeehh, maybe it'll work ¯\_(ツ)_/¯
endif

if host_machine.system() == 'emscripten'
    vfs_src += files(
        'setup_emscripten.c',
        'sync_emscripten.c',
    )
elif host_machine.system() == 'nx'
    vfs_src += files(
        'setup_switch.c',
        'sync_noop.c',
    )
else
    vfs_src += files(
        'setup_generic.c',
        'sync_noop.c',
    )
endif

if use_static_res_index
    vfs_src += files('resindex.c')
    vfs_src += resindex

    if is_developer_build
        vfs_src += files('resindex_layered.c')
    endif
endif

subdir('platform_paths')
vfs_src += vfs_platform_paths_src
