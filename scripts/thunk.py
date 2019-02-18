#!/usr/bin/env python3

import os
import sys
import runpy

assert len(sys.argv) > 1
sys.argv = sys.argv[1:]
sys.path.insert(0, os.path.dirname(sys.argv[0]))
runpy.run_path(sys.argv[0], run_name='__main__')
