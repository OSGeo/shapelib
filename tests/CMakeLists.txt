# CMake configuration for C++ unit tests

project(${CMAKE_PROJECT_NAME}Tests CXX)

# Set up GoogleTest and Benchmark
include(FetchContent)

FetchContent_Declare(
  benchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG v1.9.4
  GIT_SHALLOW TRUE
)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.17.0
  GIT_SHALLOW TRUE
)

set(BENCHMARK_DOWNLOAD_DEPENDENCIES OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(BENCHMARK_INSTALL_DOCS OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(benchmark)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Disable building GMock
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)

# Do not install GTest 
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

set_target_properties(gtest gtest_main benchmark benchmark_main PROPERTIES FOLDER "tests/third-party")

foreach(executable dbf_test sbn_test shp_test shp_bench)
  add_executable(${executable} ${PROJECT_SOURCE_DIR}/${executable}.cc)
  target_link_libraries(${executable} PRIVATE ${PACKAGE} gtest benchmark)
  add_test(
    NAME ${executable}
    COMMAND ${executable}
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
  )
  target_compile_features(${executable} PUBLIC cxx_std_17)
  if("${executable}" MATCHES ".*_bench$")
    set(exec_folder "tests/bench")
  else()
    set(exec_folder "tests")
  endif()  
  set_target_properties(${executable} PROPERTIES FOLDER ${exec_folder} CXX_EXTENSIONS OFF)
  if (BUILD_SHARED_LIBS AND ${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.21" AND (WIN32 OR CYGWIN))
    add_custom_command(
      TARGET ${executable} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:${executable}> $<TARGET_FILE_DIR:${executable}>
      COMMAND_EXPAND_LISTS
    )
  endif()
endforeach()

configure_file(
  ${CMAKE_SOURCE_DIR}/cmake/shapelib.gta.runsettings.in
  ${CMAKE_BINARY_DIR}/shapelib.gta.runsettings
  @ONLY
)
