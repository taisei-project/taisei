
from . import common

import sys
import subprocess
import shlex


class VerionFormatError(common.TaiseiError):
    pass


class Version(object):
    def __init__(self, version_str):
        version_str = version_str.lstrip("v").replace("+", "-")
        vparts = version_str.split("-")[0].split(".")
        vextra = version_str.split("-")

        if len(vextra) > 1:
            vextra = vextra[1]
        else:
            vextra = "0"

        if len(vparts) > 3:
            raise VerionFormatError("Error: Too many dot-separated elements in version string '{0}'. Please use the following format: [v]major[.minor[.patch]][-tweak[-extrainfo]]".format(version_str))
        elif len(vparts[0]) == 0 or not vextra.isnumeric():
            raise VerionFormatError("Error: Invalid version string '{0}'. Please use the following format: [v]major[.minor[.patch]][-tweak[-extrainfo]]".format(version_str))

        while len(vparts) < 3:
            vparts.append(0)

        self.major = int(vparts[0])
        self.minor = int(vparts[1])
        self.patch = int(vparts[2])
        self.tweak = int(vextra)
        self.string = version_str
        self.full_string = "Taisei v{0}".format(version_str)

    def format(self, template='{string}'):
        return template.format(**self.__dict__)


def get(*, rootdir=None, fallback=None, args=common.default_args):
    rootdir = rootdir if rootdir is not None else args.rootdir
    fallback = fallback if fallback is not None else args.fallback_version

    if rootdir is None:
        import pathlib
        rootdir = pathlib.Path(__file__).parent

    try:
        version_str = subprocess.check_output(
            shlex.split('git describe --dirty --match v[0-9]*[!asz]'),
            cwd=str(rootdir),
            universal_newlines=True
        ).strip()

        if '-' in version_str:
            version_str += '-' + subprocess.check_output(
                shlex.split('git rev-parse --abbrev-ref HEAD'),
                cwd=str(rootdir),
                universal_newlines=True
            ).strip()
    except (subprocess.SubprocessError, OSError) as e:
        if not fallback:
            raise

        print(e, file=sys.stderr)
        print("Warning: git not found or not a git repository; using fallback version {0}".format(fallback), file=sys.stderr)
        version_str = fallback

    return Version(version_str)


def main(args):
    import argparse

    parser = argparse.ArgumentParser(description='Print the Taisei version.', prog=args[0])

    parser.add_argument('format', type=str, nargs='?', default='{string}',
        help='format string; variables: major, minor, patch, tweak, string, full_string')

    common.add_common_args(parser)

    args = parser.parse_args(args[1:])
    print(get(args=args).format(template=args.format))
