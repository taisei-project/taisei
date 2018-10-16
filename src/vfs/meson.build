
vfs_src = files(
    'nodeapi.c',
    'pathutil.c',
    'private.c',
    'public.c',
    'readonly_wrapper.c',
    'setup.c',
    'syspath_public.c',
    'union.c',
    'union_public.c',
    'vdir.c',
    'zipfile_public.c',
)

if taisei_deps.contains(dep_zip)
    vfs_src += files(
        'zipfile.c',
        'zippath.c',
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
