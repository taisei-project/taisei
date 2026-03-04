
# i18n module doesn't check for these properly
find_program('msgfmt', required: true)
find_program('xgettext', required: true)

i18n = import('i18n')

xgettext_args = [
'--from-code=utf-8',
'-kN_',
'-kF_',
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

install_l10n = (not package_data and host_machine.system() != 'emscripten')

l10n_pkg_name = '10-l10n'
l10n_subdir_name = 'locale'
l10n_build_dir = meson.current_build_dir()

l10n_mo_targets = i18n.gettext(meson.project_name(),
    install : install_l10n,
    install_dir : data_path / '@0@.pkgdir'.format(l10n_pkg_name) / l10n_subdir_name,
    args : xgettext_args,
)[0]

# NOTE: packaging and installation handled in resources/meson.build
