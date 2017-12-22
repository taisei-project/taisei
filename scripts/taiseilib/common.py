
from contextlib import contextmanager
from pathlib import Path

import subprocess


class TaiseiError(RuntimeError):
    pass


class DefaultArgs(object):
    def __init__(self):
        self.fallback_version = None
        self.rootdir = Path(__file__).parent.parent.parent
        self.depfile = None


default_args = DefaultArgs()


def add_common_args(parser, *, depfile=False):
    parser.add_argument('--fallback-version',
        default=default_args.fallback_version,
        help='fallback version string, used if git lookup fails'
    )

    parser.add_argument('--rootdir',
        type=Path,
        default=default_args.rootdir,
        help='Taisei source directory'
    )

    if depfile:
        parser.add_argument('--depfile',
            type=Path,
            default=default_args.depfile,
            help='generate a depfile for the build system'
        )


def run_main(func, args=None):
    if args is None:
        import sys
        args = sys.argv

    return func(args)


def write_depfile(depfile, target, deps):
    with open(depfile, "w") as df:
        l = [target + ":"] + list(deps) + [__file__]
        df.write(" \\\n ".join(l))


def update_text_file(outpath, data):
    import io

    try:
        with open(outpath, "r+t") as outfile:
            contents = outfile.read()

            if contents == data:
                return

            outfile.seek(0)
            outfile.write(data)
            outfile.truncate()
    except (FileNotFoundError, io.UnsupportedOperation):
        with open(outpath, "w") as outfile:
            outfile.write(data)


def meson(*args, **kwargs):
    # TODO: locate meson executable somehow
    return subprocess.check_call(('meson',) + args, **kwargs)


def meson_introspect(*args, **kwargs):
    # TODO: locate meson executable somehow
    import json
    return json.loads(subprocess.check_output(('meson', 'introspect') + args, **kwargs).decode('utf8'))


def ninja(*args, **kwargs):
    # TODO: locate ninja executable somehow
    return subprocess.check_call(('ninja',) + args, **kwargs)


@contextmanager
def in_dir(dir):
    import os

    old = Path.cwd()
    os.chdir(str(dir))

    try:
        yield
    finally:
        os.chdir(str(old))
