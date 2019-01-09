function(add executable source_files)
  add_executable(${executable} ${source_files})
  target_link_libraries(${executable}
    lambda
    libgtest
    libgmock
  )

  add_test(NAME ${executable} COMMAND ${executable})
endfunction()

#copy necessary files for testing
file(COPY ../testdata DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# All Tests#######################################
set(MODULETEST_NAME "moduletest")
set(MODULETEST_SRC
  ModuleTest.cpp
  TestUtils.cpp
  main.cpp)

set(CONFIGTEST_NAME "configurationtest")
set(CONFIGTEST_SRC
  ConfigReaderTest.cpp
  TestUtils.cpp
  main.cpp)

set(COMPRESSIONTEST_NAME "compressiontest")
set(COMPRESSIONTEST_SRC
  CompressionTest.cpp
  TestUtils.cpp
  main.cpp)

set(DECODERTEST_NAME "imagedecodertest")
set(DECODERTEST_SRC
  ImageDecoderTest.cpp
  TestUtils.cpp
  main.cpp)

set(DCTEST_NAME "distortioncorrectiontest")
set(DCTEST_SRC
  DistortionCorrectorTest.cpp
  TestUtils.cpp
  main.cpp)

#add tests
add("${MODULETEST_NAME}" "${MODULETEST_SRC}")
add("${CONFIGTEST_NAME}" "${CONFIGTEST_SRC}")
add("${COMPRESSIONTEST_NAME}" "${COMPRESSIONTEST_SRC}")
add("${DECODERTEST_NAME}" "${DECODERTEST_SRC}")
add("${DCTEST_NAME}" "${DCTEST_SRC}")
