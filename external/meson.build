
gamecontrollerdb_name = 'gamecontrollerdb.txt'
gamecontrollerdb_relpath = join_paths('gamecontrollerdb', gamecontrollerdb_name)
gamecontrollerdb_path = join_paths(meson.current_source_dir(), gamecontrollerdb_relpath)

if host_machine.system() != 'emscripten'
    install_data(
        gamecontrollerdb_relpath,
        install_dir : data_path,
        install_tag : res_install_tag,
    )
endif
