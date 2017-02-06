file(GLOB_RECURSE TEST_SOURCES "*.cpp")
file(GLOB_RECURSE LIB_SOURCES "${PROJECT_SOURCE_DIR}/src/*.cpp")
file(GLOB TEST_MOCK_SOURCES "${PROJECT_SOURCE_DIR}/test/mock/*.cpp")

set(TEST_SOURCES "${TEST_SOURCES}"
                 "${LIB_SOURCES}"
                 "${TEST_MOCK_SOURCES}"
				 "${CMAKE_COMMON_PATH}/src/NetworkInterface.cpp"
                 "${CMAKE_COMMON_PATH}/src/FilesOperation.cpp"
                 "${CMAKE_COMMON_PATH}/src/Utils.cpp")
              
message(STATUS "testing_source: ${TEST_SOURCES}:${PROJECT_SOURCE_DIR}")

add_executable(${PROJECT_LAMBDA_TEST} ${TEST_SOURCES})

target_link_libraries(${PROJECT_LAMBDA_TEST} boost_system boost_thread dl pthread stdc++ cppunit z)

add_test(${PROJECT_LAMBDA_TEST} ${PROJECT_LAMBDA_TEST})

#run the test after all the targets are built
add_custom_command(TARGET ${PROJECT_LAMBDA_TEST}
                   POST_BUILD
                   COMMAND ./${PROJECT_LAMBDA_TEST}
                   WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH}
                   COMMENT "Running ${PROJECT_LAMBDA_TEST}" VERBATIM)
