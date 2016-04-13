cmake_minimum_required(VERSION 2.8)
project(lambda)

set(MAJOR_VERSION 0)
set(MINOR_VERSION 3)
set(SO_VERSION 1)
set(LIBRARY_VERSION "${MAJOR_VERSION}.${MINOR_VERSION}")


get_filename_component(SOURCE_PARENT_DIR ${PROJECT_SOURCE_DIR} PATH)
set(CMAKE_COMMON_PATH "${SOURCE_PARENT_DIR}/libcommon")

if(NOT EXISTS ${CMAKE_COMMON_PATH})
	message(FATAL_ERROR "libcommon is not existing!\nPlease put libcommon under ${SOURCE_PARENT_DIR}/\n")
#or Input libcommon path by using variable CMAKE_COMMON_PATH")
endif()

#include directories
include_directories(
	"${CMAKE_COMMON_PATH}/include/"
	"${PROJECT_SOURCE_DIR}/include/"
	"/usr/local/include"
)

#set install path
#set(CMAKE_INSTALL_PREFIX "${SOURCE_PARENT_DIR}/install")
message(STATUS "Install directory is: ${CMAKE_INSTALL_PREFIX}")

set(EXECUTABLE_OUTPUT_PATH "${SOURCE_PARENT_DIR}/bin")
set(LIBRARY_OUTPUT_PATH "${SOURCE_PARENT_DIR}/bin")
set(CMAKE_INSTALL_LIB_PATH "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_INSTALL_INC_PATH "${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME}")

#find boost library
set(BOOST_MIN_VERSION "1.41.0")
find_package(Boost ${BOOST_MIN_VERSION} REQUIRED)

if(Boost_FOUND)
	include_directories(${Boost_INCLUDE_DIRS})
endif()

#check compiler ID
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	#message(${CMAKE_CXX_COMPILER_ID})
	#message(${CMAKE_CXX_COMPILER_VERSION})
    # require at least gcc 4.4
    #if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.4)
    #    message(FATAL_ERROR "GCC version must be at least 4.4!")
    #endif()
else()
	message(FATAL_ERROR "The script is only tested under GNU g++ compiler")
endif()

#add all header and  source files from ./libcommon directory
file(GLOB COMMON_SRC
		  "${CMAKE_COMMON_PATH}/src/*.cpp")

#file(GLOB COMMON_INC
#		  "${CMAKE_COMMON_PATH}/include/*.h")

set(COMMON_HEADERS "${CMAKE_COMMON_PATH}/include/DetInterface.h"
				   "${CMAKE_COMMON_PATH}/include/Globals.h")

#define build type, by default is release
if (NOT CMAKE_BUILD_TYPE)
		message(STATUS "No build type selected, default to RELEASE")
		set(CMAKE_BUILD_TYPE "RELEASE")
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")


#set gcc flags
if(CMAKE_COMPILER_IS_GNUCXX)
	if("${CMAKE_BUILD_TYPE}" STREQUAL "RELEASE")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -std=c++0x -fPIC -pthread -D_REENTRANT -DLDEBUG")
	elseif("${CMAKE_BUILD_TYPE}" STREQUAL "DEBUG")		
		 set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -std=c++0x -fPIC -pthread -D_REENTRANT -DLDEBUG")		
	endif()
endif()

add_subdirectory("include")
add_subdirectory("src")
#add_subdirectory("test")
