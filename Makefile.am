## Process this file with automake to produce Makefile.in

SUBDIRS = . contrib

ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = dist-zip

if PLATFORM_WIN32
no_undefined = -no-undefined
endif

# Extra files to distribute in the source tarball
EXTRA_DIST = makefile.vc CMakeLists.txt autogen.sh \
		shapelib.def \
		cmake/CMakeLists.txt \
		cmake/contrib.cmake \
		cmake/project-config-version.cmake.in \
		cmake/project-config.cmake.in \
		cmake/shapelib.gta.runsettings.in \
		cmake/shapelib.pc.cmake.in \
		tests/CMakeLists.txt \
		tests/dbf_test.cc \
		tests/sbn_test.cc \
		tests/shp_bench.cc \
		tests/shp_test.cc \
		tests/test1.sh tests/test2.sh tests/test3.sh \
		tests/expect1.out tests/expect2.out tests/expect3.out \
		tests/shape_eg_data/3dpoints.dbf \
		tests/shape_eg_data/3dpoints.shp \
		tests/shape_eg_data/3dpoints.shx \
		tests/shape_eg_data/anno.aux \
		tests/shape_eg_data/anno.dbf \
		tests/shape_eg_data/anno.shp \
		tests/shape_eg_data/anno.shx \
		tests/shape_eg_data/brklinz.dbf \
		tests/shape_eg_data/brklinz.shp \
		tests/shape_eg_data/brklinz.shx \
		tests/shape_eg_data/mexico/cities.dbf \
		tests/shape_eg_data/mexico/cities.sbn \
		tests/shape_eg_data/mexico/cities.sbx \
		tests/shape_eg_data/mexico/cities.shp \
		tests/shape_eg_data/mexico/cities.shx \
		tests/shape_eg_data/csah.dbf \
		tests/shape_eg_data/csah.shp \
		tests/shape_eg_data/csah.shx \
		tests/shape_eg_data/mexico/drainage.dbf \
		tests/shape_eg_data/mexico/drainage.sbn \
		tests/shape_eg_data/mexico/drainage.sbx \
		tests/shape_eg_data/mexico/drainage.shp \
		tests/shape_eg_data/mexico/drainage.shx \
		tests/shape_eg_data/mexico/lakes.dbf \
		tests/shape_eg_data/mexico/lakes.sbn \
		tests/shape_eg_data/mexico/lakes.sbx \
		tests/shape_eg_data/mexico/lakes.shp \
		tests/shape_eg_data/mexico/lakes.shx \
		tests/shape_eg_data/masspntz.dbf \
		tests/shape_eg_data/masspntz.shp \
		tests/shape_eg_data/masspntz.shx \
		tests/shape_eg_data/mpatch3.dbf \
		tests/shape_eg_data/mpatch3.shp \
		tests/shape_eg_data/mpatch3.shx \
		tests/shape_eg_data/multipatch.dbf \
		tests/shape_eg_data/multipatch.shp \
		tests/shape_eg_data/multipatch.shx \
		tests/shape_eg_data/multipnt.dbf \
		tests/shape_eg_data/multipnt.shp \
		tests/shape_eg_data/multipnt.shx \
		tests/shape_eg_data/pline.dbf \
		tests/shape_eg_data/pline.shp \
		tests/shape_eg_data/pline.shx \
		tests/shape_eg_data/polygon.dbf \
		tests/shape_eg_data/polygon.shp \
		tests/shape_eg_data/polygon.shx \
		tests/shape_eg_data/mexico/rivers.dbf \
		tests/shape_eg_data/mexico/rivers.sbn \
		tests/shape_eg_data/mexico/rivers.sbx \
		tests/shape_eg_data/mexico/rivers.shp \
		tests/shape_eg_data/mexico/rivers.shx \
		tests/shape_eg_data/mexico/roads.dbf \
		tests/shape_eg_data/mexico/roads.sbn \
		tests/shape_eg_data/mexico/roads.sbx \
		tests/shape_eg_data/mexico/roads.shp \
		tests/shape_eg_data/mexico/roads.shx \
		tests/shape_eg_data/mexico/roads_rt.dbf \
		tests/shape_eg_data/mexico/roads_rt.sbn \
		tests/shape_eg_data/mexico/roads_rt.sbx \
		tests/shape_eg_data/mexico/roads_rt.shp \
		tests/shape_eg_data/mexico/roads_rt.shx \
		tests/shape_eg_data/mexico/states.dbf \
		tests/shape_eg_data/mexico/states.sbn \
		tests/shape_eg_data/mexico/states.sbx \
		tests/shape_eg_data/mexico/states.shp \
		tests/shape_eg_data/mexico/states.shx \
		tests/shape_eg_data/CoHI_GCS12.dbf \
		tests/shape_eg_data/CoHI_GCS12.prj \
		tests/shape_eg_data/CoHI_GCS12_README.txt \
		tests/shape_eg_data/CoHI_GCS12.sbn \
		tests/shape_eg_data/CoHI_GCS12.sbx \
		tests/shape_eg_data/CoHI_GCS12.shp \
		tests/shape_eg_data/CoHI_GCS12.shx \
		web/maptools.css \
		web/codepage.html \
		web/index.html \
		web/shapelib-tools.html \
		web/shp_api.html \
		web/release.html \
		web/dbf_api.html \
		web/license.html \
		web/manifest.html \
		LICENSE-LGPL LICENSE-MIT \
		README.tree README.CMake

# pkg-config file
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = shapelib.pc

# Library
lib_LTLIBRARIES = libshp.la
libshp_la_includedir = $(includedir)
libshp_la_include_HEADERS = shapefil.h
noinst_HEADERS = shapefil_private.h
libshp_la_SOURCES = shpopen.c dbfopen.c safileio.c shptree.c sbnsearch.c
libshp_la_LDFLAGS = -version-info $(SHAPELIB_SO_VERSION) $(no_undefined) $(LIBM)

# Installed executables
bin_PROGRAMS = dbfadd dbfcreate dbfdump shpadd shpcreate shpdump shprewind shptreedump shputils

dbfadd_SOURCES = dbfadd.c
dbfadd_LDADD = $(top_builddir)/libshp.la

dbfcreate_SOURCES = dbfcreate.c
dbfcreate_LDADD = $(top_builddir)/libshp.la

dbfdump_SOURCES = dbfdump.c
dbfdump_LDADD = $(top_builddir)/libshp.la

shpadd_SOURCES = shpadd.c
shpadd_LDADD = $(top_builddir)/libshp.la

shpcreate_SOURCES = shpcreate.c
shpcreate_LDADD = $(top_builddir)/libshp.la

shpdump_SOURCES = shpdump.c
shpdump_LDADD = $(top_builddir)/libshp.la

shprewind_SOURCES = shprewind.c
shprewind_LDADD = $(top_builddir)/libshp.la

shptreedump_SOURCES = shptreedump.c
shptreedump_LDADD = $(top_builddir)/libshp.la

shputils_SOURCES = shputils.c
shputils_LDADD = $(top_builddir)/libshp.la

# Non-installed executables
noinst_PROGRAMS = shptest
shptest_SOURCES = shptest.c
shptest_LDADD = $(top_builddir)/libshp.la

# Tests
TESTS_ENVIRONMENT = top_builddir=$(abs_top_builddir)
# tests/test1.sh requires ftp://gdal.velocet.ca/pub/outgoing/shape_eg_data.zip
TESTS = tests/test1.sh tests/test2.sh tests/test3.sh

clean-local:
	-rm -f test*.* s*.out
