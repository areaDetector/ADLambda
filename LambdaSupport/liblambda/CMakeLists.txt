cmake_minimum_required(VERSION 2.8)
project(lambda)
enable_testing()


set(MAJOR_VERSION 1)
set(MINOR_VERSION 0)
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
set(CMAKE_INSTALL_INC_PATH "${CMAKE_INSTALL_PREFIX}/include/fsdetector/lambda")
message(STATUS "install path is:${CMAKE_INSTALL_PREFIX}")

#add module
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")
message(STATUS "module path is:${CMAKE_MODULE_PATH}")

#find necessary packages
find_package(LibFSDETCore)
find_package(LibBoost)
find_package(Zlib)

#for hardware compression
find_package(AHA)

include_directories("${PROJECT_SOURCE_DIR}/include/"
                     ${Boost_INCLUDE_DIRS}
                     ${Z_INC_DIRS}
					 ${FSDETCORE_LIB_INCLUDE_DIRS}
                     ${AHA_INC_DIRS})

link_directories(${FSDETCORE_LIB_LIBRARY_DIRS})

# check compiler ID
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    #require at least gcc 4.9
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.9)
      message(FATAL_ERROR "GCC version must be at least 4.9!")
    endif()
else()
    message(FATAL_ERROR "Unsupported compiler")
endif()

if(NOT AHA_LIB_FOUND)
  # add common flags
  add_definitions("-Wall -std=c++11 -fPIC -pthread -D_REENTRANT")
  message(STATUS "HW compression is disabled")
else(NOT AHA_LIB_FOUND)
  # add common flags
  add_definitions("-Wall -std=c++11 -fPIC -pthread -D_REENTRANT -DENABLEHWCOMPRESSION")
  message(STATUS "HW compression is enabled")
endif(NOT AHA_LIB_FOUND)

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
set(SHARED_LINKER_FLAGS
  ${Boost_LIBRARIES}
  ${FSDETCORE_LIB_LIBRARIES}
  ${AHA_LIB_DIRS}
  ${Z_LIBRARIES}
  pthread)

set(PKG_LIBS "-lboost_system -lboost_thread -lpthread -lz")

if(AHA_LIB_FOUND)
  set(PKG_LIBS "${PKG_LIBS} -laha3xxahagz_api64")
endif(AHA_LIB_FOUND)

#src
add_subdirectory("include")
add_subdirectory("src")
add_subdirectory("test")
