#config file
#generate version header file
#create and install pkg-config file
configure_file(${CMAKE_MODULE_PATH}/Version.h.cmake
                ${PROJECT_SOURCE_DIR}/include/Version.h @ONLY)


set(HEADERS Globals.h
            FilesOperation.h
			NetworkImplementation.h
			NetworkInterface.h
			ThreadUtils.h
			MemUtils.h
			MemUtils.tpp
			Utils.h
			Version.h)
		  

install(FILES ${HEADERS} DESTINATION "${CMAKE_INSTALL_INC_PATH}")
