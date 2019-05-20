#! /usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import codecs

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

#
# Release process setup see:
# https://github.com/pypa/twine
#
# Run pip install --user keyring
#
# Run on cmd.exe and then type your password when prompted
# keyring set https://upload.pypi.org/legacy/ your-username
#
# Run this to build the `dist/PACKAGE_NAME-xxx.tar.gz` file
#     rm -rf ./dist && python3 setup.py sdist
#
# Run this to build & upload it to `pypi`, type addons_zz when prompted.
#     twine upload dist/*
#
# All in one command:
#     rm -rf ./dist && python3 setup.py sdist && twine upload dist/* && rm -rf ./dist
#

try:
    # https://stackoverflow.com/questions/30700166/python-open-file-error
    with codecs.open( "README.md", 'r', errors='ignore' ) as file:
        readme_contents = file.read()

except Exception as error:
    readme_contents = ""
    sys.stderr.write( "Warning: Could not open README.md due %s" % error )


setup(
        name= 'fastfilepackage',
        version= '1.0.4',
        description = 'An module written with pure Python C Extensions to open a file and cache the more recent accessed lines',
        author = 'Evandro Coan',
        license = "LGPLv2.1",
        url = 'https://github.com/evandrocoan/fastfilewrapper',
        ext_modules= [
        Extension(
                'fastfilepackage',
                [
                    'fastfile.cpp',
                    'fastfilewrapper.cpp'
                ]
            )
        ]
    )

