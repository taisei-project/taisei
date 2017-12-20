class TaiseiError(RuntimeError):
    pass


class DefaultArgs(object):
    def getattr(self):
        return None


default_args = DefaultArgs()


def add_common_args(parser, *, depfile=False):
    parser.add_argument('--fallback-version', type=str, default=None,
        help='fallback version string, used if git lookup fails')

    parser.add_argument('--rootdir', type=str, default=None,
        help='Taisei source directory')

    if depfile:
        parser.add_argument('--depfile', type=str, default=None,
            help='generate a depfile for the build system')


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
    import subprocess
    return subprocess.check_call(('meson',) + args, **kwargs)


def meson_introspect(*args, **kwargs):
    # TODO: locate meson executable somehow
    import subprocess
    return subprocess.check_output(('meson', 'introspect') + args, **kwargs)


def ninja(*args, **kwargs):
    # TODO: locate ninja executable somehow
    import subprocess
    return subprocess.check_call(('ninja',) + args, **kwargs)
