
eventloop_src = files(
    'eventloop.c',
)

if host_machine.system() == 'emscripten'
    eventloop_src += files(
        'executor_emscripten.c',
    )
else
    eventloop_src += files(
        'executor_synchro.c',
    )
endif
