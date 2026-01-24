tests = [
    'format_strings',
]

foreach testname : tests
    e = executable(
        testname, '@0@.c'.format(testname),
        dependencies : libtaisei_dep,
        include_directories : test_incdir,
        install : false,
    )
    test(testname, e)
endforeach
