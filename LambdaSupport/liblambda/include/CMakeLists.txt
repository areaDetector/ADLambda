set(HEADER_FILES LambdaGlobals.h
				 LambdaInterface.h
				 LambdaSysImpl.h)

#install header files
install(FILES ${HEADER_FILES} DESTINATION "${CMAKE_INSTALL_INC_PATH}")

