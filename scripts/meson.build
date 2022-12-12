
ver_fb = get_option('version_fallback').strip()
version_fallback = ver_fb != '' ? ver_fb : meson.project_version()

common_taiseilib_args = [
    '--rootdir', meson.project_source_root(),
    '--fallback-version', version_fallback
]

common_taiseilib_defs = [
    '-DMESON_BUILD_ROOT=@0@'.format(meson.project_build_root()),
    '-DMESON_SOURCE_ROOT=@0@'.format(meson.project_source_root()),
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

eolconv_script = find_program(files('eolconv.py'))
eolconv_command = [eolconv_script]

package_system_name = host_machine.system()

if package_system_name == 'darwin'
    package_system_name = 'macOS'   # could also be iOS etc., but we don't support that.
elif package_system_name == 'nx'
    package_system_name = 'switch'
endif

host_eol_style = package_system_name == 'windows' ? 'crlf' : 'lf'

dist_version_string = taisei_version_string

if is_developer_build
    dist_version_string = '@0@-devbuild'.format(dist_version_string)
endif

if is_debug_build
    dist_version_string = '@0@-debug'.format(dist_version_string)
endif

dist_script = find_program(files('gen-dist.py'))
dist_name = 'Taisei-@0@-@1@-@2@'.format(dist_version_string, package_system_name, host_machine.cpu_family())
dist_command = [dist_script, common_taiseilib_args]
dist_prefix_args = ['--prefix=@0@'.format(dist_name)]

zip_filename = '@0@.zip'.format(dist_name)
zip_outpath = join_paths(meson.project_build_root(), zip_filename)
zip_command = [dist_command, '--format=zip', meson.project_build_root(), zip_outpath]

txz_filename = '@0@.tar.xz'.format(dist_name)
txz_outpath = join_paths(meson.project_build_root(), txz_filename)
txz_command = [dist_command, '--format=xztar', dist_prefix_args, meson.project_build_root(), txz_outpath]

tgz_filename = '@0@.tar.gz'.format(dist_name)
tgz_outpath = join_paths(meson.project_build_root(), tgz_filename)
tgz_command = [dist_command, '--format=gztar', dist_prefix_args, meson.project_build_root(), tgz_outpath]

tbz_filename = '@0@.tar.bz2'.format(dist_name)
tbz_outpath = join_paths(meson.project_build_root(), tbz_filename)
tbz_command = [dist_command, '--format=bztar', dist_prefix_args, meson.project_build_root(), tbz_outpath]

tar_filename = '@0@.tar'.format(dist_name)
tar_outpath = join_paths(meson.project_build_root(), tar_filename)
tar_command = [dist_command, '--format=tar', dist_prefix_args, meson.project_build_root(), tar_outpath]

dmg_script = find_program(files('macos-gen-dmg.py'))
dmg_filename = '@0@.dmg'.format(dist_name)
dmg_command = [dmg_script, join_paths(meson.project_build_root(), dmg_filename), meson.project_build_root()]

angle_enabled = get_option('install_angle')

if package_system_name == 'windows'
    nsis_script = find_program(files('win-gen-nsis.py'))
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

    nsis_command = [
        nsis_script,
        common_taiseilib_args,
        common_taiseilib_defs,
        nsis_defs,
        meson.build_root(),
        files('taisei.nsi.in')
    ]
endif

gen_atlas_script = find_program(files('gen-atlas.py'))
gen_atlas_command = [gen_atlas_script]

gen_atlases_script = find_program(files('gen-atlases.py'))
gen_atlases_command = [gen_atlases_script]

upkeep_script = find_program(files('upkeep.py'))
upkeep_command = [upkeep_script, common_taiseilib_args]
upkeep_target = run_target('upkeep', command: upkeep_command)

postconf_script = find_program(files('dump-build-options.py'))
postconf_command = [postconf_script]

pack_script = find_program(files('pack.py'))
pack_command = [pack_script, common_taiseilib_args]

glob_script = find_program(files('glob-search.py'))
glob_command = [glob_script]

check_submodules_script = find_program(files('check-submodules.py'))
check_submodules_command = [check_submodules_script]

em_set_bundle_uuid_script = find_program(files('em-set-bundle-uuid.py'))
em_set_bundle_uuid_command = [em_set_bundle_uuid_script]

fix_path_script = find_program(files('unfuck-path.py'))
fix_path_command = [fix_path_script]

format_array_script = find_program(files('format-array.py'))
format_array_command = [format_array_script]
