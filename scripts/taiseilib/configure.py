
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


pattern_with_macros = re.compile(r'\${(\w+(?:\(.*?\))?)}', re.A)
pattern_without_macros = re.compile(r'\${(\w+)}', re.A)


def make_macros(args=common.default_args):
    from . import version
    vobj = version.get(args=args)

    def macro_version(format):
        return vobj.format(format)

    return {
        'VERSION': macro_version,
    }


def configure(source, variables, *, default=None, use_macros=True, args=common.default_args):
    if use_macros:
        pattern = pattern_with_macros
        macros = make_macros(args)
    else:
        pattern = pattern_without_macros

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


def main(args):
    def parse_definition(defstring):
        return defstring.split('=', 1)

    import argparse

    parser = argparse.ArgumentParser(description='Generate a text file based on a template.', prog=args[0])

    parser.add_argument('input', help='the template file')
    parser.add_argument('output', help='the output file')

    parser.add_argument('--define', '-D',
        type=parse_definition,
        action='append',
        metavar='VARIABLE=VALUE',
        dest='variables',
        default=[],
        help='define a variable that may appear in template, can be used multiple times'
    )

    common.add_common_args(parser, depfile=True)

    args = parser.parse_args(args[1:])
    args.variables = dict(args.variables)

    configure_file(args.input, args.output, args.variables, args=args)

    if args.depfile is not None:
        common.write_depfile(args.depfile, args.output, [args.input, __file__])
