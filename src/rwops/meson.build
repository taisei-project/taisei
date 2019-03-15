
rwops_src = files(
    'rwops_autobuf.c',
    'rwops_crc32.c',
    'rwops_dummy.c',
    'rwops_ro.c',
    'rwops_segment.c',
    'rwops_zlib.c',
)

if taisei_deps.contains(dep_zip)
    rwops_src += files(
        'rwops_zipfile.c',
    )
endif
