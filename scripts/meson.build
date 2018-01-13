
if get_option('buildtype').contains('debug')
    dist_version_string = '@0@-@1@'.format(taisei_version_string, get_option('buildtype'))
else
    dist_version_string = taisei_version_string
endif

if macos_app_bundle
    dmg_command = find_program('./macos-gen-dmg.py')
    dmg_filename = 'Taisei-@0@-macOS.dmg'.format(dist_version_string)
    dmg_target = run_target('dmg',
        command: [dmg_command, join_paths(meson.build_root(), dmg_filename), meson.build_root()],
    )
endif

if host_machine.system() == 'windows'
    nsis_command = find_program('./win-gen-nsis.py')

    nsis_filename = 'Taisei-@0@-setup-@1@.exe'.format(dist_version_string, host_machine.cpu_family())
    nsis_outpath = join_paths(meson.build_root(), nsis_filename)

    nsis_defs = [
        '-DOUTPUT=@0@'.format(nsis_outpath),
    ]

    if host_machine.cpu_family() == 'x86_64'
        nsis_defs += '-DTAISEI_64BIT=1'
    else
        nsis_defs += '-DTAISEI_64BIT=0'
    endif

    nsis_target = run_target('nsis',
        command: [
            nsis_command,
            common_taiseilib_args,
            common_taiseilib_defs,
            nsis_defs,
            meson.build_root(),
            files('taisei.nsi.in')
        ],
    )
endif

zip_command = find_program('./gen-zip.py')

zip_filename = 'Taisei-@0@-@1@-@2@.zip'.format(dist_version_string, host_machine.system(), host_machine.cpu_family())
zip_outpath = join_paths(meson.build_root(), zip_filename)

zip_target = run_target('zip',
    command: [zip_command, meson.build_root(), zip_outpath],
)
