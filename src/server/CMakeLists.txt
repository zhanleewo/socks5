PROJECT(Server CXX)

SET(HEAD_LIST
	../common/list.h
	../common/common.h
	../common/ringbuffer.h
	../common/threadpool.h
	server.h
	socks5-handle.h)

SET(SRC_LIST
	main.c server.c 
	socks5-handle.c)

MESSAGE(STATUS ${CMAKE_BINARY_DIR})
MESSAGE(STATUS ${PROJECT_BINARY_DIR})
INCLUDE_DIRECTORIES(../common .)
MESSAGE(STATUS ${CMAKE_BINARY_DIR}/lib)
LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/lib)
ADD_EXECUTABLE(Server ${HEAD_LIST} ${SRC_LIST})
TARGET_LINK_LIBRARIES(Server libCommon.a)

