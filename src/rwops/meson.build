
rwops_src = files(
    'rwops_autobuf.c',
    'rwops_crc32.c',
    'rwops_dummy.c',
    'rwops_segment.c',
    'rwops_zlib.c',
)

if taisei_deps.contains(dep_zip)
    rwops_src += files(
        'rwops_zipfile.c',
    )
endif

if have_posix
    rwops_src += files('rwops_pipe_posix.c')
else
    rwops_src += files('rwops_pipe_null.c')
endif

