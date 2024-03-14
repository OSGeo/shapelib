# CMake configuration for SHP C++ unit tests

project(${CMAKE_PROJECT_NAME}Tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set up GoogleTest
include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.14.0
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

foreach(executable dbf_test)
  add_executable(${executable} ${PROJECT_SOURCE_DIR}/${executable}.cc)
  target_link_libraries(${executable} PRIVATE ${PACKAGE} gtest)
  add_test(
    NAME ${executable}
    COMMAND ${executable}
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
  )
  set_target_properties(${executable} PROPERTIES FOLDER "tests")
endforeach()
