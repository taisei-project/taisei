
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

# Workaround for Meson bug: https://github.com/mesonbuild/meson/issues/4947
python_thunk = find_program(files('thunk.py'))

version_script = files('version.py')
version_command = [python_thunk, version_script, common_taiseilib_args]

taisei_version_result = run_command(version_command, '{string}')
assert(taisei_version_result.returncode() == 0, 'Version script failed')

if taisei_version_result.stderr() != ''
    warning(taisei_version_result.stderr().strip())
endif

taisei_version_string = taisei_version_result.stdout().strip()

preprocess_script = files('configure-file.py')
# generators are string supremacists apparently...
preprocess_script_path = join_paths(meson.source_root(), '@0@'.format(preprocess_script[0]))
preprocess_args = [preprocess_script_path] + common_taiseilib_args + common_taiseilib_defs
preprocess_command = [python_thunk, preprocess_args]

preprocessor = generator(preprocess_command[0],
    arguments: preprocess_args + ['@EXTRA_ARGS@', '--depfile', '@DEPFILE@', '@INPUT@', '@OUTPUT@'],
    output: '@BASENAME@',
    depfile: '@BASENAME@.d',
)

eolconv_script = files('eolconv.py')
eolconv_command = [python_thunk, eolconv_script]

host_eol_style = host_machine.system() == 'windows' ? 'crlf' : 'lf'

if get_option('buildtype').contains('debug')
    dist_version_string = '@0@-@1@'.format(taisei_version_string, get_option('buildtype'))
else
    dist_version_string = taisei_version_string
endif

if macos_app_bundle
    dmg_script = files('macos-gen-dmg.py')
    dmg_filename = 'Taisei-@0@-macOS.dmg'.format(dist_version_string)
    dmg_target = run_target('dmg',
        command: [python_thunk, dmg_script, join_paths(meson.build_root(), dmg_filename), meson.build_root()],
    )
endif

angle_enabled = get_option('install_angle')

if host_machine.system() == 'windows'
    nsis_script = files('win-gen-nsis.py')
    nsis_command = [python_thunk, nsis_script]

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

zip_script = files('gen-zip.py')
zip_command = [python_thunk, zip_script]
zip_filename = 'Taisei-@0@-@1@-@2@.zip'.format(dist_version_string, host_machine.system(), host_machine.cpu_family())
zip_outpath = join_paths(meson.build_root(), zip_filename)
zip_target = run_target('zip',
    command: [zip_command, meson.build_root(), zip_outpath],
)

gen_atlas_script = files('gen-atlas.py')
gen_atlas_command = [python_thunk, gen_atlas_script]

gen_atlases_script = files('gen-atlases.py')
gen_atlases_command = [python_thunk, gen_atlases_script]

upkeep_script = files('upkeep.py')
upkeep_command = [python_thunk, upkeep_script, common_taiseilib_args]
upkeep_target = run_target('upkeep', command: upkeep_command)

postconf_script = files('dump-build-options.py')
postconf_command = [python_thunk, postconf_script]

pack_script = files('pack.py')
pack_command = [python_thunk, pack_script, common_taiseilib_args]

glob_script = files('glob-search.py')
glob_command = [python_thunk, glob_script]

check_submodules_script = files('check-submodules.py')
check_submodules_command = [python_thunk, check_submodules_script]
