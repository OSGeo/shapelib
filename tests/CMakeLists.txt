# CMake configuration for SHP C++ unit tests

project(${CMAKE_PROJECT_NAME}Tests CXX)

# Set up GoogleTest
include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.15.2
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Disable building GMock
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)

# Do not install GTest 
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

foreach(executable dbf_test sbn_test shp_test)
  add_executable(${executable} ${PROJECT_SOURCE_DIR}/${executable}.cc)
  target_link_libraries(${executable} PRIVATE ${PACKAGE} gtest)
  add_test(
    NAME ${executable}
    COMMAND ${executable}
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
  )
  target_compile_features(${executable} PUBLIC cxx_std_17)
  set_target_properties(${executable} PROPERTIES FOLDER "tests" CXX_EXTENSIONS OFF)
endforeach()

configure_file(
  ${CMAKE_SOURCE_DIR}/cmake/shapelib.gta.runsettings.in
  ${CMAKE_BINARY_DIR}/shapelib.gta.runsettings
  @ONLY
)
