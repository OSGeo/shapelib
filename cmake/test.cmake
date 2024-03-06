include(CTest)

if(BUILD_TESTING)
  # Set up tests:

  enable_testing()

  # Other executables to be built to facilitate tests.
  foreach(executable shptest shputils)
    add_executable(${executable} ${CMAKE_SOURCE_DIR}/${executable}.c)
    target_link_libraries(${executable} PRIVATE ${PACKAGE})
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
        ${BASH_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test1.sh ${CMAKE_SOURCE_DIR}/tests/expect1.out ${EG_DATA}
    )
    declare_test_executable(test1 "shpdump;dbfdump")
  endif()

  add_test(
    NAME test2
    COMMAND
      ${BASH_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test2.sh ${CMAKE_SOURCE_DIR}/tests/expect2.out
  )
  declare_test_executable(test2 "dbfadd;dbfcreate;dbfdump;shpadd;shpcreate;shpdump;shptest")

  add_test(
    NAME test3
    COMMAND
      ${BASH_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tests/test3.sh ${CMAKE_SOURCE_DIR}/tests/expect3.out
  )
  declare_test_executable(test3 "dbfadd;dbfcreate;dbfdump;shpadd;shpcreate;shpdump")
endif()

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
    add_executable(${executable} ${CMAKE_SOURCE_DIR}/tests/${executable}.cc)
    target_link_libraries(${executable} PRIVATE ${PACKAGE} Catch2::Catch2WithMain)
    add_test(
      NAME ${executable}
      COMMAND ${executable}
      WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/tests"
    )
    set_target_properties(${executable} PROPERTIES FOLDER "tests")
  endforeach()
endif()
