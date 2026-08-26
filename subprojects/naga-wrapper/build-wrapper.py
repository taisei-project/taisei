#!/usr/bin/env python3

import subprocess
import shutil
import pathlib
import typing


def format_depfile(target, deps):
    l = [str(target) + ':'] + [str(d) for d in deps] + [str(pathlib.Path(__file__).resolve())]
    return ' \\\n '.join(l)


def main(argv: list[str]):
    import argparse

    p = argparse.ArgumentParser(prog=argv[0])
    _ = p.add_argument('target_dir', type=pathlib.Path)
    _ = p.add_argument('profile', type=str)
    _ = p.add_argument('--rust-target', type=str)
    _ = p.add_argument('--lib-output', type=pathlib.Path)
    _ = p.add_argument('--depfile', type=pathlib.Path)
    _ = p.add_argument('--lib-name', default='libwgslcrap')
    _ = p.add_argument('--verbose', action='store_true')

    args = p.parse_args(argv[1:])
    # print(args)

    target_dir: pathlib.Path = args.target_dir
    target_dir.mkdir(parents=True, exist_ok=True)

    cargo_cmd = ['cargo', 'build',
        '--target-dir', target_dir.resolve(),
        '--profile', args.profile,
    ]

    if args.rust_target is not None:
        cargo_cmd += ['--target', args.rust_target]

    if args.verbose:
        cargo_cmd += ['--verbose']

    # print(cargo_cmd)
    subprocess.run(cargo_cmd, cwd=pathlib.Path(__file__).parent, check=True)

    libname: str = args.lib_name
    artifacts_subdir = 'debug' if args.profile == 'dev' else args.profile  # just rust things
    artifacts_path: pathlib.Path = args.target_dir / artifacts_subdir
    assert artifacts_path.is_dir()

    if args.lib_output is not None:
        orig_lib_path = artifacts_path / f'{libname}.a'
        orig_dep_path = artifacts_path / f'{libname}.d'

        args.lib_output.unlink(missing_ok=True)

        try:
            args.lib_output.hardlink_to(orig_lib_path)
        except (pathlib.UnsupportedOperation, NotImplementedError):
            _ = shutil.copy2(orig_lib_path, args.lib_output)

        if args.depfile is not None:
            orig_deps = orig_dep_path.read_text().strip()
            extra_deps = format_depfile(args.lib_output, [orig_lib_path])

            depfile_contents = '\n'.join((orig_deps, extra_deps))
            args.depfile.write_text(depfile_contents)

    return 0


if __name__ == '__main__':
    import sys
    sys.exit(main(sys.argv))
