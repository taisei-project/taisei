
from contextlib import contextmanager
from pathlib import Path

import os
import subprocess
import concurrent.futures


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


def inject_taiseilib_path():
    tl = str(Path(__file__).parent.resolve())
    pp = os.environ.get('PYTHONPATH', '').split(os.pathsep)

    if tl not in pp:
        pp.insert(0, tl)
        os.environ['PYTHONPATH'] = os.pathsep.join(pp)


def run_main(func, args=None):
    inject_taiseilib_path()

    if args is None:
        import sys
        args = sys.argv

    return func(args)


def write_depfile(depfile, target, deps):
    with Path(depfile).open('w') as df:
        l = [str(target) + ":"] + list(str(d) for d in deps) + [__file__]
        df.write(" \\\n ".join(l))


def update_text_file(outpath, data):
    import io

    try:
        with open(str(outpath), "r+t") as outfile:
            contents = outfile.read()

            if contents == data:
                return

            outfile.seek(0)
            outfile.write(data)
            outfile.truncate()
    except (FileNotFoundError, io.UnsupportedOperation):
        with open(str(outpath), "w") as outfile:
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


def wait_for_futures(futures):
    results = []

    for future in concurrent.futures.as_completed(futures):
        results.append(future.result())

    return results
