# The Googletest integration code is adapted from
#   https://github.com/kaizouman/gtest-cmake-example/blob/master/test/CMakeLists.txt

include(ExternalProject)
ExternalProject_Add(gtest
    URL https://github.com/google/googletest/archive/master.zip
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/googletest
    # Disable installation
    INSTALL_COMMAND ""
)
ExternalProject_Get_Property(gtest source_dir binary_dir)

# Create library targets and add dependencies/properties/includes
add_library(libgtest IMPORTED STATIC GLOBAL)
add_dependencies(libgtest gtest)
set_target_properties(libgtest PROPERTIES
    "IMPORTED_LOCATION" "${binary_dir}/googlemock/gtest/libgtest.a"
)

add_library(libgmock IMPORTED STATIC GLOBAL)
add_dependencies(libgmock gtest)
set_target_properties(libgmock PROPERTIES
    "IMPORTED_LOCATION" "${binary_dir}/googlemock/libgmock.a"
)

include_directories(
   "${source_dir}/googletest/include"
   "${source_dir}/googlemock/include"
)


#copy necessary files for testing
file(COPY testdata DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

function(add executable source_files)
  add_executable(${executable} ${source_files})
  target_link_libraries(${executable}
    fsdetcore
    libgtest
    libgmock
  )

  add_test(NAME ${executable} COMMAND ${executable})
endfunction()

# All Tests#######################################
set(FSDETCORETEST_NAME "fsdetcoretest")
set(FSDETCORETEST_SRC
  main.cpp
  FilesOperationTest.cpp
  UtilsTest.cpp
  ThreadUtilsTest.cpp
  MemUtilsTest.cpp)
add("${FSDETCORETEST_NAME}" "${FSDETCORETEST_SRC}")

# File Operations Test############################
set(FILETEST_NAME "filetest")
set(FILETEST_SRC
  FilesOperationTest.cpp
  main.cpp)
add("${FILETEST_NAME}" "${FILETEST_SRC}")


# Utils Test######################################
set(UTILSTEST_NAME "utiltest")
set(UTILSTEST_SRC
  UtilsTest.cpp
  main.cpp
)


add("${UTILSTEST_NAME}" "${UTILSTEST_SRC}")

# # ThreadUtils Test################################
set(THREADTEST_NAME "threadtest")
set(THREADTEST_SRC
  ThreadUtilsTest.cpp
  main.cpp)
add("${THREADTEST_NAME}" "${THREADTEST_SRC}")


# MemUtils Test###################################
set(MEMTEST_NAME "memtest")
set(MEMTEST_SRC
  MemUtilsTest.cpp
  main.cpp)
add("${MEMTEST_NAME}" "${MEMTEST_SRC}")

