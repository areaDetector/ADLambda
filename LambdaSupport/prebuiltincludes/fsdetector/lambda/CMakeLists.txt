#config file
#generate version header file
#create and install pkg-config file
configure_file(${CMAKE_MODULE_PATH}/Version.h.cmake
                ${PROJECT_SOURCE_DIR}/include/Version.h @ONLY)


set(HEADERS LambdaGlobals.h
            LambdaInterface.h
			LambdaSysImpl.h
			Version.h)


#install header files
install(FILES ${HEADERS} DESTINATION "${CMAKE_INSTALL_INC_PATH}")

