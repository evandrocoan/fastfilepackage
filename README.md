
## Fast File Package

An module written with pure Python C Extensions to open a file and
cache the more recent accessed lines.
See these files for an usage example:
1. [tests/fastfileperformance.py](tests/fastfileperformance.py)
1. [tests/fastfiletest.py](tests/fastfiletest.py)
1. [tests/getline_c_performance.cpp](tests/getline_c_performance.cpp)
1. [tests/getline_cpp_performance.cpp](tests/getline_cpp_performance.cpp)


## Installation

Requires `Python 3` ,
`pip3`, `distutils`, `setuptools`, `wheel` and
a `C++ 11` compiler installed:
```
sudo apt-get install build-essential g++ python3-dev python3 python3-pip
pip3 install setuptools wheel
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

### Alternative file reading

There are available 3 alternative implementations for file reading.
Both of them,
should have the same performance.
1. You can define `FASTFILE_GETLINE=0` to use the Python builtins.open() implementation (default)
1. You can define `FASTFILE_GETLINE=1` to use the C++ std::getline() implementation
1. You can define `FASTFILE_GETLINE=2` to use the POSIX C getline() implementation

Usage examples:
1. `FASTFILE_GETLINE=1 pip3 install . -v`
1. `FASTFILE_GETLINE=1 FASTFILE_DEBUG=1 pip3 install . -v`


### File reading optimizations

You can enable a file reading optimization with the environment variable `FASTFILE_REGEX=1`.
Do not define this variable or define it as `FASTFILE_REGEX=0` to disable the optimization.

1. `FASTFILE_REGEX=1` will use the C language builtin regex library `regex.h`:
   https://linux.die.net/man/3/regexec
1. `FASTFILE_REGEX=2` will use PCRE2 regex library `pcre2.h`:
   http://pcre.org/current/doc/html/pcre2_match.html,
   it requires the installation of the `libpcre2-dev` package with `sudo apt-get install libpcre2-dev`
1. `FASTFILE_REGEX=3` will use RE2 regex library `re2/re2.h`:
   https://github.com/google/re2/wiki/CplusplusAPI,
   it requires the installation of the `re2` library with:
   ```sh
   git clone https://github.com/google/re2 re2 &&
   cd re2 &&
   make &&
   sudo make install &&
   make testinstall
   ```
1. `FASTFILE_REGEX=4` will use Hyperscan regex library `hs.h`:
   https://github.com/intel/hyperscan,
   it requires the installation of the `libhyperscan-dev` package with `sudo apt-get install libhyperscan-dev`

Notes:
 * Defining the variable `FASTFILE_REGEX` only has effect when `FASTFILE_GETLINE=2` is set (as/with value 2).
   If the variable `FASTFILE_GETLINE=2` is not defined (as/with value 2),
   any definition of `FASTFILE_REGEX` is ignored.


## Debugging

You you use the `FASTFILE_DEBUG=1` variable specified on the `Enable debug mode` section,
you do not need to build the program with `CFLAGS="-O0..." CXXFLAGS="-O0 ..."
/usr/bin/pip3 install .` because these flags are already set by
`FASTFILE_DEBUG=1`.

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

In case you program enter on a deadlock (or livelock),
you can use `gdb` to discover or what is happening.
For this,
first you need get a `core dump file` of the system somehow.
Once you get the `core dump` file,
you can call `gdb program_name coredump`.
Now,
you can use the `gdb` commands to navigate between the existing threads and
to discover which of them are waiting for the other,
i.e.,e it is causing the deadlock or livelock.
1. `p / s 0x6ffffdf8ed0` print or content on the address with the format "%s"
1. `x / 100w 0x6ffffdf8ed0` "examines" the specified memory address
1. `frame 0` shows the corresponding stack frame `0` as `bt f` (`backtrace full`)
1. `bt f 5` shows the last 5 stack frames with all debugging symbols data
1. `f 0` and` p varname` print the `varname` on `frame 0` context
1. https://stackoverflow.com/questions/14659147/how-to-print-pointer-content-in-gdb
1. https://stackoverflow.com/questions/14502236/how-to-view-a-pointer-like-an-array-in-gdb

Note:
Instead of creating a `core dump file`,
you can run your program directly with `gdb --args program_name` and
once you are on the `gdb` command line,
you can use the `run` command and
once your program enters/starts a dead or live lock,
you can press `Ctrl+C` to stop the program execution.
Then,
`gdb` will already had "captured" the `core dump file`.


## Enable debug mode

The default debug level is `0` where no debugging message code is generated into the final binary.
Guaranteeing maximum performance.
If you would like to compile a binary with debug messages,
define the environment variable `FASTFILE_DEBUG` before running the installer.
You can see the debugging level available on the file [source/debugger.h](source/debugger.h):
```
FASTFILE_DEBUG=1 pip install .
FASTFILE_DEBUG=1 pip install -e .

FASTFILE_DEBUG=1 python setup.py install
FASTFILE_DEBUG=1 python setup.py develop
```

If you are on Windows,
run it like this:
```
set "FASTFILE_DEBUG=1" && pip install .
set "FASTFILE_DEBUG=1" && pip install -e .
set "FASTFILE_DEBUG=1" && python setup.py install
set "FASTFILE_DEBUG=1" && python setup.py develop
```

To debug `refcounts` leaks:
1. CPython compiled with `./configure --with-pydebug`
1. Runtime compiled with C assertion:
   crash (kill itself with SIGABRT signal) if a C assertion fails (`assert(...);`).
1. Use the debug hooks on memory allocators by default,
   as `PYTHONDEBUG=debug` environment variable:
   detect memory under and overflow and
   misuse of memory allocators.
1. Compiled without compiler optimizations (`-Og` or even `-O0`) to be usable with a debugger like `gdb`:
   `python-gdb.py` should work perfectly.
   However,
   the regular runtime is unusable with `gdb` since most variables and
   function arguments are stored in registers,
   and so `gdb` fails with the `<optimized out>` message.
1. https://pythoncapi.readthedocs.io/runtimes.html#debug-build
1. https://stackoverflow.com/questions/40121749/why-gcc-og-doesnt-generate-source-code-line-mapping
1. https://stackoverflow.com/questions/8650972/how-do-i-debug-refcounts-in-a-python-c-extension-the-easiest-way

To generate core dumps instead of stack traces on `Cygwin`
```
# Use `unset CYGWIN` to generate stackdump again
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

