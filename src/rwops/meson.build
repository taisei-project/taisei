
rwops_src = files(
    'rwops_arena.c',
    'rwops_autobuf.c',
    'rwops_crc32.c',
    'rwops_dummy.c',
    'rwops_segment.c',
    'rwops_sha256.c',
    'rwops_stdiofp.c',
    'rwops_util.c',
    'rwops_zlib.c',
    'rwops_zstd.c',
)

if is_developer_build
    rwops_src += files(
        'rwops_trace.c',
    )
endif
