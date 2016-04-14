set(PROJECT_LAMBDA_TEST lambdatest)


#copy necessary files for testing
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/filetest DESTINATION ${EXECUTABLE_OUTPUT_PATH})

include_directories("./include/"
  "./mock/")
#add_subdirectory("include")
add_subdirectory("src")
