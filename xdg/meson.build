if host_machine.system() != 'windows' and host_machine.system() != 'darwin'
    install_data('taisei-replay-viewer.desktop', 'taisei.desktop',
                 install_dir : join_paths(get_option('datadir'), 'applications'))
    install_data('taisei.png',
                 install_dir : join_paths(get_option('datadir'), 'icons/hicolor/128x128/apps'))
    install_data('taisei-replay.png',
                 install_dir : join_paths(get_option('datadir'), 'icons/hicolor/256x256/mimetypes'))
    install_data('taisei.xml',
                 install_dir : join_paths(get_option('datadir'), 'mime/packages'))
endif
