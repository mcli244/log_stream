cmake_minimum_required(VERSION 3.0.0)

project(log_stream)

set (TAGET log_stream)

set(CMAKE_CXX_STANDARD 11)

include_directories(inc)
set(SRCS
	main.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/log_stream.cpp
	
	)

set(CMAKE_INSTALL_PREFIX ${PROJECT_BINARY_DIR}/devlop)
set(CMAKE_INSTALL_LIBDIR "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_INSTALL_BINDIR "${CMAKE_INSTALL_PREFIX}/bin")
set(CMAKE_INSTALL_INCLUDEDIR "${CMAKE_INSTALL_PREFIX}/include")


find_package(OpenCV REQUIRED)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_INSTALL_BINDIR}) 
set(LIBRARY_OUTPUT_PATH ${CMAKE_INSTALL_LIBDIR})
add_executable(${TAGET} ${SRCS})


target_link_libraries(${TAGET}
	pthread
)
