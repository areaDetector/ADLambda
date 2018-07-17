set(SOURCES ImageDecoder.cpp
            LambdaModule.cpp
			LambdaSysImpl.cpp
			LambdaTask.cpp
			LambdaConfigReader.cpp
			Compression.cpp
			CompressionContext.cpp
			ZlibWrapper.cpp
			LambdaDataReceiver.cpp
			LambdaDataRecvFactory.cpp)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

target_link_libraries(${PROJECT_NAME} ${SHARED_LINKER_FLAGS})
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION "${LIBRARY_VERSION}"
                                       SOVERSION "${SO_VERSION}")

#install target
install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_LIB_PATH})

#create and install pkg-config file
configure_file(${CMAKE_MODULE_PATH}/pkgconfig.pc.cmake
                ${EXECUTABLE_OUTPUT_PATH}/${PROJECT_NAME}.pc @ONLY)

install(FILES ${EXECUTABLE_OUTPUT_PATH}/${PROJECT_NAME}.pc
         DESTINATION ${CMAKE_INSTALL_LIB_PATH}/pkgconfig)
