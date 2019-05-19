from distutils.core import setup, Extension

# https://bugs.python.org/issue35893
from distutils.command import build_ext

def get_export_symbols(self, ext):
    parts = ext.name.split(".")
    if parts[-1] == "__init__":
        initfunc_name = "PyInit_" + parts[-2]
    else:
        initfunc_name = "PyInit_" + parts[-1]

build_ext.build_ext.get_export_symbols = get_export_symbols

setup(name='fastfilepackage', version='1.0',  \
      ext_modules=[Extension('fastfilepackage', ['fastfile.cpp', 'fastfilewrapper.cpp'])])
