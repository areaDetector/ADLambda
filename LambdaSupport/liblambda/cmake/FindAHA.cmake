#find AHA api

#path of AHA include
find_path(AHA_INC_DIRS NAMES ahagz_api.h aha3xx_common.h
  PATHS
  /usr/include
  /usr/local/include
  /home/yuelong/lib.local/include/aha
  )

#path of AHA path
find_library(AHA_LIB_DIRS  NAMES aha3xxahagz_api64
  PATHS
  /usr/lib/
  /usr/local/lib
  /home/yuelong/lib.local/lib64
  )



if(AHA_INC_DIRS AND AHA_LIB_DIRS)
  SET(AHA_LIB_FOUND TRUE)
endif(AHA_INC_DIRS AND AHA_LIB_DIRS)


if(NOT AHA_LIB_FOUND)
  message(STATUS "cannot find aha3xxahagz_api64")
  SET(AHA_INC_DIRS "")
  SET(AHA_LIB_DIRS "")
else(NOT AHA_LIB_FOUND)
  message(STATUS "AHA include path:${AHA_INC_DIRS}")
  message(STATUS "AHA lib path:${AHA_LIB_DIRS}")
  # ADD_LIBRARY(AHA_LIBRARIES STATIC IMPORTED)
  # SET_TARGET_PROPERTIES(AHA_LIBRARIES PROPERTIES
  #   IMPORTED_LOCATION ${AHA_LIB_DIRS})
  set(AHA_LIBRARIES "libaha3xxahagz_api64.a")

  message(STATUS "${AHA_LIBRARIES}")
endif(NOT AHA_LIB_FOUND)
