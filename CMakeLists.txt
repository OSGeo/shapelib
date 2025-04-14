# Top-level CMakeLists.txt for the CMake-based build and test system
# of the shapelib software.
#
# Copyright (C) 2012-2013, Alan W. Irwin
#
# SPDX-License-Identifier: MIT OR LGPL-2.0-or-later
#
# See README.CMake

set(PACKAGE shp)
if (TARGET ${PACKAGE})
  return ()
endif ()

# Version 3.11 or above of cmake is currently required for all platforms.
cmake_minimum_required(VERSION 3.11)
project(shapelib C CXX)

message(STATUS "CMake version = ${CMAKE_VERSION}")
message(STATUS "CMAKE_SYSTEM_NAME = ${CMAKE_SYSTEM_NAME}")

set(PROJECT_VERSION_MAJOR 1)
set(PROJECT_VERSION_MINOR 6)
set(PROJECT_VERSION_PATCH 1)
set(shp_LIB_VERSIONINFO "5:0:1")
set(PROJECT_VERSION
  "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

set(EG_DATA ${PROJECT_SOURCE_DIR}/tests/shape_eg_data CACHE STRING "")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# libraries are all shared by default.
option(BUILD_SHARED_LIBS "Build shared libraries" ON)

# Option to build executables
option(BUILD_APPS "Build executables" ON)
# Option to build contributed utilities
# Defaults to ${BUILD_APPS}. If you reconfigure with a different BUILD_APPS
# value, be aware that you have to explicitly change BUILD_SHAPELIB_CONTRIB,
# otherwise the previous value from the cache will be used.
option(BUILD_SHAPELIB_CONTRIB "Build utilities (from contrib)" ${BUILD_APPS})

# Use rpath?
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  # No rpath on Darwin. Setting it will only cause trouble.
else()
  option(USE_RPATH "Use -rpath when linking libraries, executables" ON)
endif()

# In windows all created dlls are gathered in the dll directory
# if you add this directory to your PATH all shared libraries are available
if(BUILD_SHARED_LIBS AND (WIN32 OR CYGWIN))
  set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/dll)
endif()


# Set up install locations.
set(
  CMAKE_INSTALL_BINDIR bin
  CACHE PATH "install location for user executables"
)

set(
  CMAKE_INSTALL_LIBDIR lib
  CACHE PATH "install location for object code libraries"
)

set(
  CMAKE_INSTALL_INCLUDEDIR include
  CACHE PATH "install location for C header files"
)

set(
  CMAKE_INSTALL_CMAKEDIR share/${PROJECT_NAME}
  CACHE PATH "install location for read-only architecture-independent shp data"
)

file(RELATIVE_PATH RELATIVE_LIBDIR
  ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}
  ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}
)

message(STATUS "CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR}")

set(prefix "${CMAKE_INSTALL_PREFIX}")
set(exec_prefix "\${prefix}")
if(IS_ABSOLUTE "${CMAKE_INSTALL_LIBDIR}")
  set(libdir "${CMAKE_INSTALL_LIBDIR}")
else()
  set(libdir "\${exec_prefix}/${CMAKE_INSTALL_LIBDIR}")
endif()
if(IS_ABSOLUTE "${CMAKE_INSTALL_INCLUDEDIR}")
  set(includedir "${CMAKE_INSTALL_INCLUDEDIR}")
else()
  set(includedir "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}")
endif()
set(SHAPELIB_PC ${CMAKE_CURRENT_BINARY_DIR}/shapelib.pc)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/shapelib.pc.cmake.in
  ${SHAPELIB_PC} @ONLY
)

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  # Set a default build type for single-configuration cmake generators
  # if no build type is set.
  set(CMAKE_BUILD_TYPE Release)
endif()

# Export build information to help other projects link installed
# shapelib software.  Only one of these signatures is required
# for the export_shp name.
install(EXPORT targets
  NAMESPACE ${PROJECT_NAME}::
  DESTINATION ${CMAKE_INSTALL_CMAKEDIR}
  FILE "${PROJECT_NAME}-targets.cmake"
)

# Initial boilerplate done, now build library and executables.

if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

set(lib_SRC
  shpopen.c
  dbfopen.c
  safileio.c
  shptree.c
  sbnsearch.c
  shapefil.h
  shapefil_private.h
  shapelib.def
)

add_library(${PACKAGE} ${lib_SRC})

target_include_directories(${PACKAGE}
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

include(TestBigEndian)
if(HAVE_BYTE_ORDER_BIG_ENDIAN)
  # Define SHP_BIG_ENDIAN if the system is big-endian
  add_definitions(-DSHP_BIG_ENDIAN=1)
endif()

if(MSVC)
  target_compile_options(${PACKAGE} PRIVATE /W4)
else()
  target_compile_options(${PACKAGE} PRIVATE -Wall -Wextra -Wformat-signedness -pedantic -Wno-unknown-warning-option)
endif()

if(WIN32 AND NOT CYGWIN)
  set_target_properties(${PACKAGE} PROPERTIES
    COMPILE_DEFINITIONS SHAPELIB_DLLEXPORT
  )
endif()

if(UNIX)
  find_library(M_LIB m)
  if(M_LIB)
    target_link_libraries(${PACKAGE} -lm)
  endif()
endif()

# Convert shp_LIB_VERSIONINFO libtool version format into SOVERSION
# Convert from ":" separated into CMake list format using ";"
string(REPLACE ":" ";" shp_LIB_VERSIONINFO ${shp_LIB_VERSIONINFO})
list(GET shp_LIB_VERSIONINFO 0 shp_LIB_VERSION_CURRENT)
list(GET shp_LIB_VERSIONINFO 2 shp_LIB_VERSION_AGE)
math(EXPR shp_SOVERSION "${shp_LIB_VERSION_CURRENT} - ${shp_LIB_VERSION_AGE}")
set(shp_VERSION ${PROJECT_VERSION})
set_target_properties(${PACKAGE} PROPERTIES
  SOVERSION ${shp_SOVERSION}
  VERSION ${shp_VERSION}
)

if(USE_RPATH)
  set_target_properties(${PACKAGE} PROPERTIES
    INSTALL_RPATH "${CMAKE_INSTALL_LIBDIR}"
  )
endif()

install(TARGETS ${PACKAGE}
  EXPORT targets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# executables to be built and installed.
if(BUILD_APPS)
  set(executables
    shpcreate
    shpadd
    shpdump
    shprewind
    dbfcreate
    dbfadd
    dbfdump
    shptreedump
  )
endif()

find_program(BASH_EXECUTABLE bash)
if(BASH_EXECUTABLE)
  option(BUILD_TESTING "Build tests"  ON)
else()
  set(BUILD_TESTING OFF CACHE BOOL "Disable tests")
  message(STATUS "WARNING: bash not available so disabling testing")
endif()

if(NOT MSVC)
  # Set the run time path for shared libraries for non-Windows machines.
  set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
  if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    # See also INSTALL_RPATH property on the tools.
    set(CMAKE_MACOSX_RPATH ON)
  else()
    # Use relative path so that package is relocatable
    set(CMAKE_INSTALL_RPATH "\$ORIGIN/${RELATIVE_LIBDIR}")
  endif()
endif()

foreach(executable ${executables})
  add_executable(${executable} ${executable}.c)
  target_link_libraries(${executable} ${PACKAGE})
  if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    # Ensure that the package is relocatable
    set_target_properties(${TOOLS} PROPERTIES
      INSTALL_RPATH "@loader_path/${RELATIVE_LIBDIR}")
  endif()
  if(NOT MSVC)
    target_compile_options(${executable} PRIVATE -Wall -Wextra)
  endif()
endforeach(executable ${executables})

install(TARGETS ${executables}
  EXPORT targets DESTINATION ${CMAKE_INSTALL_BINDIR})

# Install header
install(FILES shapefil.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Install pkg-config file
install(FILES "${SHAPELIB_PC}" DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

if(BUILD_TESTING)
  # Set up tests:

  enable_testing()

  # Other executables to be built to facilitate tests.
  foreach(executable shptest shputils)
    add_executable(${executable} ${executable}.c)
    target_link_libraries(${executable} PRIVATE ${PACKAGE})
    if (BUILD_SHARED_LIBS AND ${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.21" AND (WIN32 OR CYGWIN))
      add_custom_command(
        TARGET ${executable} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:${executable}> $<TARGET_FILE_DIR:${executable}>
        COMMAND_EXPAND_LISTS
      )
    endif()
  endforeach()

  # Set environment variables defining path to executables being used
  function(declare_test_executable TEST TARGETS)
    foreach(TARGET ${TARGETS})
      string(TOUPPER ${TARGET} NAME)
      list(APPEND TEST_ENV ${NAME}=$<TARGET_FILE:${TARGET}>)
    endforeach()

    set_tests_properties(${TEST} PROPERTIES ENVIRONMENT "${TEST_ENV}")
  endfunction()

  if(EG_DATA)
    add_test(
      NAME test1
      COMMAND
        ${BASH_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tests/test1.sh ${PROJECT_SOURCE_DIR}/tests/expect1.out ${EG_DATA}
    )
    declare_test_executable(test1 "shpdump;dbfdump")
  endif()

  add_test(
    NAME test2
    COMMAND
      ${BASH_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tests/test2.sh ${PROJECT_SOURCE_DIR}/tests/expect2.out
  )
  declare_test_executable(test2 "dbfadd;dbfcreate;dbfdump;shpadd;shpcreate;shpdump;shptest")

  add_test(
    NAME test3
    COMMAND
      ${BASH_EXECUTABLE} ${PROJECT_SOURCE_DIR}/tests/test3.sh ${PROJECT_SOURCE_DIR}/tests/expect3.out
  )
  declare_test_executable(test3 "dbfadd;dbfcreate;dbfdump;shpadd;shpcreate;shpdump")

  add_subdirectory(tests)
endif()

include(cmake/contrib.cmake)

add_subdirectory(cmake)
