#!/usr/bin/env python3

from taiseilib.common import (
    run_main,
    update_text_file,
)

import sys, re, argparse
from pathlib import Path


profiles = {
    'core': {
        'headers': [
            'SDL_opengl.h',
            'SDL_opengl_glext.h',
        ]
    },
    'es': {
        'headers': [
            'SDL_opengles2_gl2.h',
            'SDL_opengles2_gl2ext.h',
        ]
    }
}

regex_glcall = re.compile(r'(gl[A-Z][a-zA-Z0-9]+)\(')
regex_glproto_template = r'(^GLAPI\s+([a-zA-Z_0-9\s]+?\**)\s*((?:GL)?APIENTRY)\s+%s\s*\(\s*(.*?)\s*\);)'

def write_header(args):
    print(args)

    header = args.header
    srcdir = args.dir
    glfuncs = set(args.functions or [])
    idir = args.sdl_headers
    glheaders = '\n'.join((idir / h).read_text() for h in profiles[args.profile]['headers'])

    print(srcdir)

    for src in srcdir.glob('**/*.c'):
        for func in regex_glcall.findall(src.read_text()):
            glfuncs.add(func)

    glfuncs = sorted(glfuncs)

    typedefs = []
    prototypes = []

    for func in glfuncs:
        try:
            proto, rtype, callconv, params = re.findall(regex_glproto_template % func, glheaders, re.DOTALL|re.MULTILINE)[0]
        except IndexError:
            print("Function '%s' not found in the GL headers. Either it really does not exist, or this script is bugged. Please check the opengl headers in %s\nUpdate aborted." % (func, idir))
            exit(1)

        proto = re.sub(r'\s+', ' ', proto, re.DOTALL|re.MULTILINE)
        params = ', '.join(re.split(r',\s*', params))

        typename = 'ts%s_ptr' % func
        typedef = 'typedef %s (%s *%s)(%s);' % (rtype, callconv, typename, params)

        typedefs.append(typedef)
        prototypes.append(proto)

    text = header.read_text()

    subs = {
        'gldefs': '#define GLDEFS \\\n' + ' \\\n'.join('GLDEF(%s, ts%s, ts%s_ptr)' % (f, f, f) for f in glfuncs),
        'undefs': '\n'.join('#undef %s' % f for f in glfuncs),
        'redefs': '\n'.join('#define %s ts%s' % (f, f) for f in glfuncs),
        'reversedefs': '\n'.join('#define ts%s %s' % (f, f) for f in glfuncs),
        'typedefs': '\n'.join(typedefs),
        'protos': '\n'.join(prototypes),
    }

    for key, val in subs.items():
        text = re.sub(
            r'(// @BEGIN:%s@)(.*?)(// @END:%s@)' % (key, key),
            r'\1\n%s\n\3' % val,
            text,
            flags=re.DOTALL|re.MULTILINE
        )

    update_text_file(str(header), text)

def main(args):
    parser = argparse.ArgumentParser(description='Update a header file with prototypes of OpenGL functions used by all files in a directory', prog=args[0])

    parser.add_argument('dir',
        help='Directory containing the source files to look for OpenGL calls in',
        type=Path,
    )

    parser.add_argument('header',
        help='Header file to modify',
        type=Path,
    )

    parser.add_argument('--sdl-headers', '-I',
        help='Where to look for the SDL headers (default: /usr/include/SDL2)',
        default=Path('/usr/include/SDL2'),
        type=Path,
    )

    parser.add_argument('--profile', '-p',
        help='Which OpenGL profile to target',
        default='core',
        choices=list(profiles),
    )

    parser.add_argument('--function', '-f',
        help='Include this function even if a direct call was not found in the source files; may be specified multiple times',
        action='append',
        dest='functions',
    )

    write_header(parser.parse_args(args[1:]))

if __name__ == '__main__':
    run_main(main)
