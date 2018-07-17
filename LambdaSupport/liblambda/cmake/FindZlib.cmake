#find zlib

#path of zlib include
find_path(Z_INC_DIRS NAMES zlib.h
  PATHS
  /usr/include
  /usr/local/include
  )

#path of zlib path
find_library(Z_LIB_DIRS  NAMES z
  PATHS
  /usr/lib/
  /usr/local/lib
  )

if(Z_INC_DIRS AND Z_LIB_DIRS)
  SET(Z_LIB_FOUND TRUE)
endif(Z_INC_DIRS AND Z_LIB_DIRS)


if(NOT Z_LIB_FOUND)
  message(FATAL_ERROR "cannot find lz.")
else(NOT Z_LIB_FOUND)
  message(STATUS "Zlib include path:${Z_INC_DIRS}")
  message(STATUS "Zlib lib path:${Z_LIB_DIRS}")
  set(Z_LIBRARIES "z")
endif(NOT Z_LIB_FOUND)
  


 

