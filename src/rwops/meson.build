
rwops_src = files(
    'rwops_autobuf.c',
    'rwops_crc32.c',
    'rwops_dummy.c',
    'rwops_ro.c',
    'rwops_segment.c',
    'rwops_sha256.c',
    'rwops_zlib.c',
    'rwops_zstd.c',
)

if taisei_deps.contains(dep_zip)
    rwops_src += files(
        'rwops_zipfile.c',
    )
endif

if is_debug_build
    rwops_src += files(
        'rwops_trace.c',
    )
endif
