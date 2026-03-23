
atlases_dir = meson.current_source_dir()
atlases_config_dir = join_paths(atlases_dir, 'config')
resources_gfx_dir = join_paths(resources_pkg_main, 'gfx')

# Args are applied in this order:
#   1. Common args
#   2. Atlas-specific
#   3. Profile-specific

preset_webp = [
    '--format', 'webp',
]

preset_png = [
    '--format', 'png',
]

preset_tsatlas = [
    '--format', 'tsatlas'
]

preset_thorough = [
    '--width=512',
    '--height=512',
    '--samples=256',
    '--single',
    '--tsatlas-zstd-level=22',
]

preset_fast = [
    '--width=4096',
    '--height=4096',
    '--samples=0',
    '--multiple',
]

atlas_common_args = [
    # Padding to prevent filter bleeding.
    # 1 is normally enough, but not when mipmaps are involved.
    '--border=2'
]

atlases = [
    ['common',      []],
    ['common_ui',   []],
    ['huge',        []],
    ['portraits',   ['--tsatlas-sort=perimeter']],
]

atlas_profiles = [
    ['',        [preset_tsatlas, preset_thorough]],
    ['-fast',   [preset_tsatlas, preset_fast]],
    ['-png',    [preset_png,     preset_fast]],
    ['-webp',   [preset_webp,    preset_fast]],
]

foreach profile : atlas_profiles
    profile_suffix = profile[0]
    profile_args = profile[1]
    profile_targets = []

    foreach atlas : atlases
        atlas_name = atlas[0]
        atlas_args = atlas[1]
        atlas_target = 'gen-atlas-@0@@1@'.format(atlas_name, profile_suffix)
        profile_targets += atlas_target

        run_target(atlas_target,
            command : [
                gen_atlas_command,
                atlases_config_dir,
                join_paths(atlases_dir, atlas_name),
                resources_gfx_dir,
                atlas_common_args,
                atlas_args,
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
