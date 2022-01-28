#!/usr/bin/env python3

import sys

def quote(s):
    return '"' + s.replace('\\', '\\\\').replace('"', '\\"') + '"'

print("{", ', '.join([quote(s) for s in sys.argv[1:]] + ['NULL']), "}")
