#find libboost
set(BOOST_MIN_VERSION "1.55.0")
find_package(Boost ${BOOST_MIN_VERSION} COMPONENTS system thread REQUIRED)

if(NOT Boost_FOUND)
  message(FATAL_ERROR "Cannot find libboost.\nEither the libboost version is older than ${BOOST_MIN_VERSION} or libboost is not installed.")
endif(NOT Boost_FOUND)
