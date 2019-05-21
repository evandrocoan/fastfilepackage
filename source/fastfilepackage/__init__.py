#!/usr/bin/env python3
# -*- coding: UTF-8 -*-

# https://stackoverflow.com/questions/12856931/how-to-put-my-python-c-module-inside-package
from .version import __version__

# https://stackoverflow.com/questions/56244851/how-to-use-setuptools-packages-and-ext-modules-with-the-same-name
from . import fastfilewrapper
