set(HEADERS LambdaGlobals.h
            LambdaInterface.h
			LambdaSysImpl.h)

#install header files
install(FILES ${HEADERS} DESTINATION "${CMAKE_INSTALL_INC_PATH}")

