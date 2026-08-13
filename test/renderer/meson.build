
tests = [
    'cube',
    'texture',
    'triangle',
]

foreach testname : tests
    e = executable(
        testname, '@0@.c'.format(testname),
        dependencies : libtaisei_dep,
        include_directories : test_incdir,
        install : false,
        link_args : taisei_link_args,
    )
endforeach
