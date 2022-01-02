
filewatch_enabled = is_developer_build
filewatch_src = []

if filewatch_enabled
    # For shims like inotify-kqueue
    inotify_dep = dependency('libinotify', required : false)
    if not inotify_dep.found()
        inotify_dep = cc.find_library('inotify', required : false)
    endif

    filewatch_enabled = (
        have_posix and
        cc.has_function('inotify_init1', dependencies : inotify_dep) and
        cc.has_header_symbol('sys/inotify.h', 'struct inotify_event', dependencies : inotify_dep)
    )

    if not filewatch_enabled
        warning('No inotify support; live file monitoring not available')
    endif
endif

if filewatch_enabled
    taisei_deps += inotify_dep
    filewatch_src += files('filewatch_inotify.c')
else
    filewatch_src += files('filewatch_null.c')
endif
