
## Fast File Package

An module written with pure Python C Extensions to open a file and
cache the more recent accessed lines.
See these files for an usage example:
1. [fastfiletest.py](fastfiletest.py)
1. [fastfiletest.cpp](fastfiletest.cpp)


## Installation

Requires Python 3,
pip3, distutils and
C++ 11 compiler installed:
```
sudo apt-get install build-essential g++ python3-dev
```

Then, you can clone this repository with:
```
git clone https://github.com/evandrocoan/fastfilepackage
cd fastfilepackage
pip3 install .
```

Or install it with:
```
pip3 install fastfilepackage
```

# Performance improvements

You can have `70%` of increment if you use a Posix complaint system as Linux or Cygwin.

If you would like to disable this,
just define `USE_STD_GETLINE=1` macro by calling the installer like this,
if you are on Linux or Cygwin/Posix:
```
MY_ENVIRONMENT_VARIABLE=1 pip install .
MY_ENVIRONMENT_VARIABLE=1 pip install -e .

MY_ENVIRONMENT_VARIABLE=1 python setup.py install
MY_ENVIRONMENT_VARIABLE=1 python setup.py develop
```

But,
if you are on Windows,
run it like this:
```
set "MY_ENVIRONMENT_VARIABLE=1" && pip install .
set "MY_ENVIRONMENT_VARIABLE=1" && pip install -e .
set "MY_ENVIRONMENT_VARIABLE=1" && python setup.py install
set "MY_ENVIRONMENT_VARIABLE=1" && python setup.py develop
```


## Debugging

If Python got segmentation fault,
you need to install the debug symbol packages,
and compile the program into mode debug.
These instructions works for both Linux and
Cygwin.
```
sudo apt-get install python3-dbg
apt-cyg install libcrypt-devel python36-devel python36-debuginfo python3-debuginfo python3-cython-debuginfo
CFLAGS="-O0 -g -ggdb -fstack-protector-all" CXXFLAGS="-O0 -g -ggdb -fstack-protector-all" /usr/bin/pip3 install .

cd modulename/source
gdb --args /usr/bin/python3 -m modulename.__init__ -p ../tests/light/
python3 -m pdb -m modulename.__init__ -p ../tests/light/

cd /usr/bin
/usr/bin/python3 -u -m trace -t modulename-script.py -p ../tests/light/
```
1. https://github.com/spiside/pdb-tutorial
1. https://docs.python.org/3.7/library/pdb.html
1. https://docs.python.org/3.7/library/trace.html
1. https://stackoverflow.com/questions/1629685/when-and-how-to-use-gccs-stack-protection-feature
1. https://stackoverflow.com/questions/25678978/how-to-debug-python-script-that-is-crashing-python
1. https://stackoverflow.com/questions/46265835/how-to-debug-a-python-module-run-with-python-m-from-the-command-line

To debug `refcounts` leaks:
1. CPython compiled with ./configure --with-pydebug
1. Runtime compiled with C assertion:
   crash (kill itself with SIGABRT signal) if a C assertion fails (assert(...);).
1. Use the debug hooks on memory allocators by default,
   as PYTHONDEBUG=debug environment variable:
   detect memory under- and overflow and
   misuse of memory allocators.
1. Compiled without compiler optimizations (`-Og` or even `-O0`) to be usable with a debugger like `gdb`:
   `python-gdb.py` should work perfectly.
   However,
   the regular runtime is unusable with `gdb` since most variables and
   function arguments are stored in registers,
   and so `gdb` fails with the `<optimized out>` message.
1. https://pythoncapi.readthedocs.io/runtimes.html#debug-build
1. https://stackoverflow.com/questions/8650972/how-do-i-debug-refcounts-in-a-python-c-extension-the-easiest-way

To generate core dumps instead of stack traces
```
export CYGWIN="$CYGWIN error_start=dumper -d %1 %2"
```
1. https://stackoverflow.com/questions/320001/using-a-stackdump-from-cygwin-executable

If you see this when running `gdb`:
```
[New Thread 8980.0x626c]
[New Thread 8980.0x1ba0]
[New Thread 8980.0x2454]
[New Thread 8980.0x3fbc]
warning: the debug information found in "/usr/lib/debug//usr/bin/cygwin1.dbg" does not match "/usr/bin/cygwin1.dll" (CRC mismatch).

[New Thread 8980.0x21c4]
```
Run the command `rm /usr/lib/debug//usr/bin/cygwin1.dbg`
1. http://cygwin.1069669.n5.nabble.com/Debugging-with-GDB-td94421.html


## License

See the file [LICENSE.txt](LICENSE.txt)

