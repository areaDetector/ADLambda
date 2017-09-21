set(UNITTEST_LIB
    $ENV{GTEST_LIB_PATH}/libgtest.a
    $ENV{GTEST_LIB_PATH}/libgtest_main.a
	$ENV{GTEST_LIB_PATH}/libgmock.a
	$ENV{GTEST_LIB_PATH}/libgmock_main.a
	${SHARED_LINKER_FLAG})

include_directories($ENV{GTEST_INC_PATH}
					  ".")


#copy necessary files for testing
file(COPY testdata DESTINATION ${EXECUTABLE_OUTPUT_PATH})

function(RunTest project_name_arg source_arg library_arg)
  # message(STATUS "${project_name_arg}")
  # message(STATUS "${source_arg}")
  # message(STATUS "${library_arg}")

  # Link runTests with what we want to test and the GTest and pthread library
  add_executable(${project_name_arg} ${source_arg})
  target_link_libraries(${project_name_arg} ${library_arg})

  #run the test after all the targets are built
  add_custom_command(TARGET ${project_name_arg}
    POST_BUILD
    COMMAND ./${project_name_arg}
    WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}
    COMMENT "Running ${project_name_arg}" VERBATIM)
endfunction()

# All Tests#######################################
set(FSDETCORETEST_NAME "fsdetcoretest")
set(FSDETCORETEST_SRC
  main.cpp
  FilesOperationTest.cpp
  UtilsTest.cpp
  ThreadUtilsTest.cpp
  MemUtilsTest.cpp
  ${PROJECT_SOURCE_DIR}/src/FilesOperation.cpp
  ${PROJECT_SOURCE_DIR}/src/Utils.cpp
  ${PROJECT_SOURCE_DIR}/src/ThreadUtils.cpp)

RunTest("${FSDETCORETEST_NAME}" "${FSDETCORETEST_SRC}" "${UNITTEST_LIB}")

# # File Operations Test############################
# set(FILETEST_NAME "filetest")
# set(FILETEST_SRC
#   FilesOperationTest.cpp
#   main.cpp
#   ${PROJECT_SOURCE_DIR}/src/FilesOperation.cpp)

# RunTest("${FILETEST_NAME}" "${FILETEST_SRC}" "${UNITTEST_LIB}")

# # Utils Test######################################
# set(UTILSTEST_NAME "utiltest")
# set(UTILSTEST_SRC
#   UtilsTest.cpp
#   main.cpp
#   ${PROJECT_SOURCE_DIR}/src/Utils.cpp)

# RunTest("${UTILSTEST_NAME}" "${UTILSTEST_SRC}" "${UNITTEST_LIB}")

# # ThreadUtils Test################################
# set(THREADTEST_NAME "threadtest")
# set(THREADTEST_SRC
#   ThreadUtilsTest.cpp
#   main.cpp
#   ${PROJECT_SOURCE_DIR}/src/ThreadUtils.cpp)

# RunTest("${THREADTEST_NAME}" "${THREADTEST_SRC}" "${UNITTEST_LIB}")

# # MemUtils Test###################################
# set(MEMTEST_NAME "memtest")
# set(MEMTEST_SRC
#   MemUtilsTest.cpp
#   main.cpp)
#   #${PROJECT_SOURCE_DIR}/src/ThreadUtils.cpp)

# RunTest("${MEMTEST_NAME}" "${MEMTEST_SRC}" "${UNITTEST_LIB}")

