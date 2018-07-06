# The Googletest integration code is adapted from
#   https://github.com/kaizouman/gtest-cmake-example/blob/master/test/CMakeLists.txt

include(ExternalProject)
ExternalProject_Add(gtest
    #URL https://github.com/google/googletest/archive/master.zip
	URL https://github.com/google/googletest/archive/release-1.8.0.zip
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
#file(COPY testdata DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

#add src
add_subdirectory("src")
