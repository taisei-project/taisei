#!/usr/bin/env python3


class VerionFormatError(RuntimeError):
    pass


class Version(object):
    def __init__(self, version_str):
        version_str = version_str.lstrip("v")
        vparts = version_str.split("-")[0].split(".")
        vextra = version_str.replace("+", "-").split("-")

        if len(vextra) > 1:
            vextra = vextra[1]
        else:
            vextra = "0"

        if len(vparts) > 3:
            raise VerionFormatError("Error: Too many dot-separated elements in version string '{0}'. Please use the following format: [v]major[.minor[.patch]][-tweak[-extrainfo]]".format(version_str))
            sys.exit(1)
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


def get(fallback=None, rootdir=None):
    if rootdir is None:
        import pathlib
        rootdir = pathlib.Path(__file__).parent

    try:
        from subprocess import check_output
        git = check_output(['git', 'describe', '--tags', '--match', 'v[0-9]*[!asz]'], cwd=rootdir, universal_newlines=True)
        version_str = git.strip()
    except:
        if not fallback:
            raise

        print("Warning: git not found or not a git repository; using fallback version {0}".format(fallback), file=sys.stderr)
        version_str = fallback

    return Version(version_str)


if __name__ == '__main__':
    import sys
    import argparse

    parser = argparse.ArgumentParser(description='Print the Taisei version.')
    parser.add_argument('format', type=str, nargs='?', default='{string}',
                        help='format string; variables: major, minor, patch, tweak, string, full_string')
    parser.add_argument('--fallback', type=str, default=None,
                        help='fallback source string, used if git lookup fails')
    parser.add_argument('--rootdir', type=str, default=None,
                        help='CWD for the git command')

    args = parser.parse_args()
    print(get(fallback=args.fallback, rootdir=args.rootdir).format(template=args.format))
