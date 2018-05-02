
atlases_dir = meson.current_source_dir()
atlases_overrides_dir = join_paths(atlases_dir, 'overrides')
resources_gfx_dir = join_paths(resources_dir, 'gfx')

atlases = [
    'common',
    'common_ui',
    'huge',
    'portraits',
    'reimu',
]

atlas_common_args = [
    '--width',  '4096',
    '--height', '4096',
    '--border', '1',
    '--single',
    '--crop',
]

atlas_profiles = [
    ['', [
        '--leanify',
    ]],

    ['-fast', [
        '--no-leanify',
    ]],
]

foreach profile : atlas_profiles
    profile_suffix = profile[0]
    profile_args = profile[1]
    profile_targets = []

    foreach atlas : atlases
        atlas_target = 'gen-atlas-@0@@1@'.format(atlas, profile_suffix)
        profile_targets += atlas_target

        run_target(atlas_target,
            command : [
                gen_atlas_command,
                atlases_overrides_dir,
                join_paths(atlases_dir, atlas),
                resources_gfx_dir,
                atlas_common_args,
                profile_args,
            ],
        )
    endforeach

    run_target('gen-atlases@0@'.format(profile_suffix),
        command : [
            gen_atlases_command,
            profile_targets,
        ],
    )
endforeach
