# Fortissimo
Fortissimo is a free standing and network agnostic fronctocol framework.

For information about the design and workings of fortissimo see the wiki for our [design doc](doc/wiki/design/design.md) and [protocol information](doc/wiki//design/protocol.md).
Additional user level documentation is forthcoming.

## Building
To build Fortissimo at the moment use the ``Makefile`` and run the default target.

```sh
make
```

This will build dependencies (once), and then build the Fortissimo library and tests.

The following targets are intended for "human consumption"
 - ``make dependencies``: Builds the project dependencies in the ``lib/`` folder.
   - ``INTERNAL``: ``true`` indicates dependencies should be taken from internal gitlab, any other value indicates public github.
 - ``make configure``: Runs CMake to configure the build system. This will automatically invoke ``make dependencies`` if necessary. The following variables can be set
   - ``CXX=[g++, clang++, ...]``: change the compiler vendor. The given compiler must support C++11. The default compiler is ``g++``.
   - ``BUILD_TYPE=[Release,Debug,Test,Ci]``: Controls some of the compiler flag settings.
     - ``Release``: A build intended to be packaged for consumers.
     - ``Debug``: A build intended for developers using a debugger, it enables debug symbols.
     - ``Test`` (default): A build intended for developer use without debug symbols.
     - ``Ci``: A build intended to be ran during continuous integration. The primary difference between ``Test`` and ``Debug`` is the use of ``-Werror`` to fail a build when a compiler warning is encountered (these are usually noted, but do not fail a build).
 - ``make build``: compiles sources and generates executables. (automatically invokes ``configure`` when necessary).
 - ``make test`` (default): runs the unit test suite (automatically invokes ``build`` when necessary).
   - ``TEST_FILTER=<testgroup>.<testname>`` can filter which tests are run. Wildcards are allowed for ``<testgroup>`` or ``<testname>``.
 - ``make clean``: deletes compiled output files, causing them to be rebuilt on the next build.
 - ``make mopclean``: deletes all build system configuration files along with compiled output files.
 - ``make mrclean``: deletes all Fortissimo build files (compiled output, and configuration) as well as all dependencies.
 - ``make format``: runs ``clang-format`` to format the source code.
 - ``make stealth-format``: runs the ``stealth-clang-format`` wrapper container to format the source code.
 - ``make debug-test`` or: This is a convenience to run the test suite under the debugger (specifically ``gdb``). This is interactive, through ``gdb``. It works best when ``BUILD_TYPE=Debug``. It can use the following variable:
  - ``TEST_FILTER=<testgroup>.<testname>`` can filter which tests being run under the debugger. Wildcards are allowed for ``<testgroup>`` or ``<testname>``.

## Running
At the moment there is example code written under the ``example/`` directory.
Thorough explanations of these examples, along with instructions to run them will be provided at a later date.
