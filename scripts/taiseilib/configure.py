
from . import common

import re


class ConfigureError(common.TaiseiError):
    pass


class NameNotDefinedError(ConfigureError):
    def __init__(self, type, name):
        return super().__init__("{} '{}' is not defined".format(type, name))


class VariableNotDefinedError(NameNotDefinedError):
    def __init__(self, name):
        return super().__init__('Variable', name)


class MacroNotDefinedError(NameNotDefinedError):
    def __init__(self, name):
        return super().__init__('Macro', name)


def make_macros(args=common.default_args):
    from . import version
    vobj = version.get(args=args)

    def macro_version(format):
        return vobj.format(format)

    def macro_file(path):
        with open(path, 'r') as infile:
            return infile.read()

    return {
        'VERSION': macro_version,
        'FILE': macro_file,
    }


def configure(source, variables, *, default=None, use_macros=True, prefix='${', suffix='}', args=common.default_args):
    prefix = re.escape(prefix)
    suffix = re.escape(suffix)

    if use_macros:
        pattern = re.compile(prefix + r'(\w+(?:\(.*?\))?)' + suffix, re.A)
        macros = make_macros(args)
    else:
        pattern = re.compile(prefix + r'(\w+)' + suffix, re.A)

    def sub(match):
        var = match.group(1)

        if var[-1] == ')':
            macro_name, macro_arg = var.split('(', 1)
            macro_arg = macro_arg[:-1]

            try:
                macro = macros[macro_name]
            except KeyError:
                raise MacroNotDefinedError(var)

            val = macro(macro_arg)
        else:
            try:
                val = variables[var]
            except KeyError:
                if default is not None:
                    val = default

                raise VariableNotDefinedError(var)

        return str(val)

    return pattern.sub(sub, source)


def configure_file(inpath, outpath, variables, **options):
    with open(inpath, "r") as infile:
        result = configure(infile.read(), variables, **options)

    common.update_text_file(outpath, result)


def add_configure_args(parser):
    def parse_definition(defstring):
        return defstring.split('=', 1)

    def parse_definition_from_file(defstring):
        var, fpath = parse_definition(defstring)

        with open(fpath, 'r') as infile:
            return (var, infile.read())

    parser.add_argument('--define', '-D',
        type=parse_definition,
        action='append',
        metavar='VARIABLE=VALUE',
        dest='variables',
        default=[],
        help='define a variable that may appear in template, can be used multiple times'
    )

    parser.add_argument('--define-from-file', '-F',
        type=parse_definition_from_file,
        action='append',
        metavar='VARIABLE=FILE',
        dest='variables',
        default=[],
        help='like --define, but the value is read from the specified file'
    )

    parser.add_argument('--prefix',
        default='${',
        help='prefix for substitution expressions, default: ${'
    )

    parser.add_argument('--suffix',
        default='}',
        help='suffix for substitution expressions, default: }'
    )


def main(args):
    import argparse

    parser = argparse.ArgumentParser(description='Generate a text file based on a template.', prog=args[0])

    parser.add_argument('input', help='the template file')
    parser.add_argument('output', help='the output file')

    add_configure_args(parser)
    common.add_common_args(parser, depfile=True)

    args = parser.parse_args(args[1:])
    args.variables = dict(args.variables)

    configure_file(args.input, args.output, args.variables,
        prefix=args.prefix,
        suffix=args.suffix,
        args=args
    )

    if args.depfile is not None:
        common.write_depfile(args.depfile, args.output, [args.input, __file__])
