
platform_src = []
platform_deps = []

if host_machine.system() == 'windows'
    platform_src += files(
        'choice_dialog_win32.c',
    )
else
    platform_src += files(
        'choice_dialog_stub.c',  # I hope we'll never need this for non-windows platforms
    )
endif

if dep_gamemode.found()
    platform_src += files('gamemode.c')
    platform_deps += dep_gamemode
else
    platform_src += files('gamemode_stub.c')
endif

if host_machine.system() == 'nx'
    platform_src += files('arch_switch.c')
endif
