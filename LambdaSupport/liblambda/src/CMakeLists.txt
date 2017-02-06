set(SOURCES	ImageDecoder.cpp
			LambdaModule.cpp
			LambdaSysImpl.cpp
			LambdaTask.cpp
			LambdaConfigReader.cpp
			Compression.cpp
			CompressionContext.cpp
			ZlibWrapper.cpp)

add_library(${PROJECT_NAME} SHARED ${COMMON_SRC} ${SOURCES})

target_link_libraries(${PROJECT_NAME} boost_system boost_thread dl pthread stdc++ z)
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION "${LIBRARY_VERSION}"
                                       SOVERSION "${SO_VERSION}")

#install target
install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_LIB_PATH})

#install include files
install(FILES ${DEV_HEADERS} DESTINATION "${CMAKE_INSTALL_INC_PATH}")
