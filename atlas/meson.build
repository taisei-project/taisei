
atlases_dir = meson.current_source_dir()
atlases_overrides_dir = join_paths(atlases_dir, 'overrides')

# TODO: find a nice way to scale this over multiple atlas directories, if needed.

run_target('gen-atlases',
    command: [
        gen_atlases_command,
        atlases_overrides_dir,
        join_paths(atlases_dir, 'common'),
        join_paths(resources_dir, 'gfx'),
        '--name',   'common',
        '--width',  '4096',
        '--height', '4096',
        '--border', '1',
        '--single',
        '--crop',
        '--leanify',
    ],
)

run_target('gen-atlases-fast',
    command: [
        gen_atlases_command,
        atlases_overrides_dir,
        join_paths(atlases_dir, 'common'),
        join_paths(resources_dir, 'gfx'),
        '--name',   'common',
        '--width',  '4096',
        '--height', '4096',
        '--border', '1',
        '--single',
        '--crop',
        '--no-leanify',
    ],
)
