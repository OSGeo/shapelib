The CMakeLists.txt file in the current directory is a complete build
system for shapelib which does everything that the shapelib Makefile
does and Linux and the shapelib makefile.vc file does on Windows with
a lot more flexibility.  For example, full testing can be done with
the present CMake-based approach because an optional and configurable
location is used for the downloadable (for example, wget
http://dl.maptools.org/dl/shapelib/shape_eg_data.zip) eg_data tree
that is used for all the "stream1" tests.

To use this build system on Unix or MinGW/MSYS/Windows:

(1) (Optional) Download eg_data from the location above.

(2) Download shapelib-1.6.0.tar.gz from http://download.osgeo.org/shapelib
and unpack it

(3) Copy the CMakeLists.txt file into the top-level of the unpacked
shapelib-1.6.0 source tree.

(4) Create a separate empty build tree and use it to configure, build,
install and test shapelib.  For example (you will need to tailor the
compiler, compile options, install prefix, eg_data location, and source tree location to your own
needs):

# Configure with the compiler and compiler options of your choice.
# N.B. the gcc -fvisibility=hidden option not (yet) supported by shapelib.
env CC=gcc CFLAGS="-O3 -Wuninitialized" \
cmake \
-S /path/to/shapelib-1.6.0
-B build_dir
-G "Unix Makefiles" \
-DCMAKE_INSTALL_PREFIX=/home/software/shapelib/install \
-DEG_DATA=/path/to/eg_data/

# Build and install
cmake --build build_dir -- VERBOSE=1 -j4 install

# Test
ctest --test-dir build_dir

The -DEG_DATA option is optional, but if you don't specify
the eg_data directory that way the stream1 tests will be dropped.

Note the above procedure is what you should do on a Unix platform like
Linux where the generator -G "Unix Makefiles" works well.  On
MINGW/MSYS the procedure is essentially the same except you should use
the -G "MSYS Makefiles" cmake option instead to specify a good generator
for that platform.

I have used variants of the above procedure to create, test, and
install shapelib on both the Linux and MinGW/MSYS/Wine platforms.
Furthermore, on both platforms I have built and tested PLplot using
the installed versions created by the above procedure.  No issues were
discovered with PLplot example 19 (which demos PLplot map capabilities with
map shapefiles) for these two separate platform tests.
