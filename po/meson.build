i18n = import('i18n')

xgettext_args = [
'--from-code=utf-8',
'-kN_',
'-kMSG:2',
'-kMSG_UNSKIPPABLE:3',
'-kT_NARRATOR',
'-kT_REIMU',
'-kT_MARISA',
'-kT_YOUMU',
'-kT_YUKARI',
'-kT_KANAKO',
'-kT_SUWAKO',
'-kT_SANAE',
'-kT_SUMIREKO',
'-kT_YUYUKO',
'-kT_ELLY',
'-kT_KURUMI',
'-kT_REMILIA',
'-kT_FLANDRE',
'-kT_PATCHOULI',
]

l10n_pkg_name = '10-l10n'
l10n_subdir_name = 'locale'
l10n_build_dir = meson.current_build_dir()

l10n_mo_targets = i18n.gettext(meson.project_name(),
    install : not package_data,
    install_dir : data_path / '@0@.pkgdir'.format(l10n_pkg_name) / l10n_subdir_name,
    args : xgettext_args,
)[0]

if package_data
    l10n_pkg_zip = '@0@.zip'.format(l10n_pkg_name)
    bindist_deps += custom_target(l10n_pkg_zip,
        command : [pack_command,
            l10n_build_dir,
            '@OUTPUT@',
            '--prefix', l10n_subdir_name,
            '--exclude', '*.zip',
            '--depfile', '@DEPFILE@',
        ],
        output : l10n_pkg_zip,
        depends : l10n_mo_targets,
        depfile : '@0@.d'.format(l10n_pkg_zip),
        install : true,
        install_dir : data_path,
        install_tag : res_install_tag,
        console : true,
    )
else
    bindist_deps += l10n_mo_targets
endif
