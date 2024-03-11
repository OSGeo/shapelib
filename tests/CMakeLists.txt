project(${CMAKE_PROJECT_NAME}Tests CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(BUILD_TESTING)
  # Set up Catch2
  include(FetchContent)

  FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.3
  )

  FetchContent_MakeAvailable(Catch2)

  foreach(executable dbf_test)
    add_executable(${executable} ${PROJECT_SOURCE_DIR}/${executable}.cc)
    target_link_libraries(${executable} PRIVATE ${PACKAGE} Catch2::Catch2WithMain)
    add_test(
      NAME ${executable}
      COMMAND ${executable}
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    )
    set_target_properties(${executable} PROPERTIES FOLDER "tests")
  endforeach()
endif()
