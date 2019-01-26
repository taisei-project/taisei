
ver_fb = get_option('version_fallback').strip()
version_fallback = ver_fb != '' ? ver_fb : meson.project_version()

common_taiseilib_args = [
    '--rootdir', meson.source_root(),
    '--fallback-version', version_fallback
]

common_taiseilib_defs = [
    '-DMESON_BUILD_TYPE=@0@'.format(get_option('buildtype')),
    '-DMESON_BUILD_ROOT=@0@'.format(meson.build_root()),
    '-DMESON_SOURCE_ROOT=@0@'.format(meson.source_root()),
]

version_script = find_program(files('version.py'))
version_command = [version_script, common_taiseilib_args]

taisei_version_result = run_command(version_command, '{string}', check : true)

if taisei_version_result.stderr() != ''
    warning(taisei_version_result.stderr().strip())
endif

taisei_version_string = taisei_version_result.stdout().strip()

preprocess_script = find_program(files('configure-file.py'))
preprocess_args = common_taiseilib_args + common_taiseilib_defs
preprocess_command = [preprocess_script, preprocess_args]
preprocessor = generator(preprocess_script,
    arguments: preprocess_args + ['@EXTRA_ARGS@', '--depfile', '@DEPFILE@', '@INPUT@', '@OUTPUT@'],
    output: '@BASENAME@',
    depfile: '@BASENAME@.d',
)

eolconv_command = find_program(files('eolconv.py'))
host_eol_style = host_machine.system() == 'windows' ? 'crlf' : 'lf'

if get_option('buildtype').contains('debug')
    dist_version_string = '@0@-@1@'.format(taisei_version_string, get_option('buildtype'))
else
    dist_version_string = taisei_version_string
endif

if macos_app_bundle
    dmg_command = find_program(files('macos-gen-dmg.py'))
    dmg_filename = 'Taisei-@0@-macOS.dmg'.format(dist_version_string)
    dmg_target = run_target('dmg',
        command: [dmg_command, join_paths(meson.build_root(), dmg_filename), meson.build_root()],
    )
endif

if host_machine.system() == 'windows'
    nsis_command = find_program(files('win-gen-nsis.py'))

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

    if angle_enabled
        nsis_defs += '-DUSE_ANGLE=1'
    else
        nsis_defs += '-DUSE_ANGLE=0'
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

zip_command = find_program(files('gen-zip.py'))
zip_filename = 'Taisei-@0@-@1@-@2@.zip'.format(dist_version_string, host_machine.system(), host_machine.cpu_family())
zip_outpath = join_paths(meson.build_root(), zip_filename)
zip_target = run_target('zip',
    command: [zip_command, meson.build_root(), zip_outpath],
)

gen_atlas_command = find_program(files('gen-atlas.py'))
gen_atlases_command = find_program(files('gen-atlases.py'))

upkeep_script = find_program(files('upkeep.py'))
upkeep_command = [upkeep_script, common_taiseilib_args]
upkeep_target = run_target('upkeep', command: upkeep_command)

postconf_script = find_program(files('dump-build-options.py'))
# meson.add_postconf_script(postconf_script.path())
run_command(postconf_script)

pack_script = find_program(files('pack.py'))
pack_command = [pack_script, common_taiseilib_args]

glob_command = find_program(files('glob.py'))
