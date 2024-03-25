if not install_xdg
    subdir_done()
endif

icon_sizes = [512, 256, 128, 64, 32]

install_data(
    'org.taisei_project.Taisei.tsr.desktop',
    'org.taisei_project.Taisei.desktop',
    install_dir : xdg_path / 'applications',
    install_tag : 'xdg'
)

install_data(
    'org.taisei_project.Taisei.xml',
    install_dir : xdg_path / 'mime/packages',
    install_tag : 'xdg'
)

install_data(
    'org.taisei_project.Taisei.appdata.xml',
    install_dir : xdg_path / 'metainfo',
    install_tag : 'xdg'
)

foreach size : icon_sizes
    apps_dir = xdg_path / f'icons/hicolor/@size@x@size@/apps'
    mimetypes_dir = xdg_path / f'icons/hicolor/@size@x@size@/mimetypes'

    install_data(
        f'icons/taisei-@size@.png',
        rename : 'org.taisei_project.Taisei.png',
        install_dir : apps_dir,
        install_tag : 'xdg'
    )
    install_symlink(
        'taisei.png',
        pointing_to : 'org.taisei_project.Taisei.png',
        install_dir : apps_dir,
        install_tag : 'xdg'
    )
    install_data(
        f'icons/replay-@size@.png',
        rename : 'org.taisei_project.Taisei.tsr.png',
        install_dir : mimetypes_dir,
        install_tag : 'xdg'
    )
    install_symlink(
        'taisei-replay.png',
        pointing_to : 'org.taisei_project.Taisei.tsr.png',
        install_dir : mimetypes_dir,
        install_tag : 'xdg'
    )
endforeach
