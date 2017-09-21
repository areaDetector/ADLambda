set(HEADERS Globals.h
            FilesOperation.h
			NetworkImplementation.h
			NetworkInterface.h
			ThreadUtils.h
			MemUtils.h
			MemUtils.tpp
			Utils.h)

install(FILES ${HEADERS} DESTINATION "${CMAKE_INSTALL_INC_PATH}")
