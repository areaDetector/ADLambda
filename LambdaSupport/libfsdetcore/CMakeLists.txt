cmake_minimum_required(VERSION 2.8)
project(fsdetcore)
enable_testing()


set(MAJOR_VERSION 0)
set(MINOR_VERSION 9)
set(LIBRARY_VERSION "${MAJOR_VERSION}.${MINOR_VERSION}")
set(SO_VERSION ${LIBRARY_VERSION})
# configure path
#binary output path
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_SOURCE_DIR}/bin")
message(STATUS "executable output path is:${EXECUTABLE_OUTPUT_PATH}")

#lib output path
set(LIBRARY_OUTPUT_PATH "${CMAKE_SOURCE_DIR}/bin")
message(STATUS "library output path is:${LIBRARY_OUTPUT_PATH}")

#install path
set(CMAKE_INSTALL_LIB_PATH "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_INSTALL_INC_PATH
    "${CMAKE_INSTALL_PREFIX}/include/fsdetector/core")

message(STATUS "install path is:${CMAKE_INSTALL_PREFIX}")

#add module
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")
message(STATUS "module path is:${CMAKE_MODULE_PATH}")

#find necessary packages
find_package(LibBoost)

include_directories("${CMAKE_SOURCE_DIR}/include"
                      ${Boost_INCLUDE_DIRS})

# get_property(inc_dirs DIRECTORY PROPERTY INCLUDE_DIRECTORIES)
# message("-- inc_dirs = ${inc_dirs}")

# check compiler ID
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    #require at least gcc 4.9
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9)
      message(FATAL_ERROR "GCC version must be at least 4.9!")
    endif()
else()
    message(FATAL_ERROR "Unsupported compiler")
endif()

# add common flags
add_definitions("-Wall -std=c++11 -fPIC -pthread -D_REENTRANT")

# define build type, by default is release
if ("${CMAKE_BUILD_TYPE}" STREQUAL "DEBUG")
        message(STATUS "Setting build type to DEBUG")
		set(CMAKE_CXX_FLAGS_DEBUG "-O2 -g")
		set(CMAKE_BUILD_TYPE "DEBUG")
else()
        set(CMAKE_CXX_FLAGS_RELEASE "-O3")
        set(CMAKE_BUILD_TYPE "RELEASE")
		
endif()	  

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

#add shared linker flags
set(SHARED_LINKER_FLAG ${Boost_LIBRARIES} pthread)
set(PKG_LIBS "-lboost_system -lboost_thread -lpthread")

#src
add_subdirectory("include")
add_subdirectory("src")

# unittest
add_subdirectory("test")
