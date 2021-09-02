
a_stream_src = files(
    'mixer.c',
    'player.c',
    'stream.c',
    'stream_opus.c',
    'stream_pcm.c',
)

dep_opusfile = dependency('opusfile', required : true, static : static, fallback : ['opusfile', 'opusfile_dep'])

a_stream_deps = []
a_stream_libdeps = [dep_opusfile]
