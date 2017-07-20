cmake_minimum_required(VERSION 3.5)
project(obs-virtualsource)

find_package(FFmpeg COMPONENTS swscale REQUIRED)

set(virtualsource_SOURCES
	dllmain.cpp
	virtual-cam.cpp
	virtual-audio.cpp
	../queue/share_queue.cpp
	)

set(virtualsource_HEADERS
	virtual-cam.h
	virtual-audio.h
	../queue/share_queue.h
	)
	
set(DShowPath "../../dshowdeps/include")

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	find_library(DShowLib strmbase PATHS "../../dshowdeps/lib/x64" NO_DEFAULT_PATH)
	find_library(DShowLibD strmbasd PATHS "../../dshowdeps/lib/x64" NO_DEFAULT_PATH)
else()
	find_library(DShowLib strmbase PATHS "../../dshowdeps/lib/x86" NO_DEFAULT_PATH)
	find_library(DShowLibD strmbasd PATHS "../../dshowdeps/lib/x86" NO_DEFAULT_PATH)
endif()

add_library(obs-virtualsource MODULE
	${virtualsource_SOURCES}
	${virtualsource_HEADERS})
	
include_directories(
	${FFMPEG_INCLUDE_DIRS}
	${DShowPath})
		
target_link_libraries(obs-virtualsource
	${FFMPEG_LIBRARIES}
	${DShowLibD}
	winmm
	Strmiids
	)
	
add_definitions(-DDEBUG)
SET(CMAKE_MODULE_LINKER_FLAGS_DEBUG "${CMAKE_MODULE_LINKER_FLAGS_DEBUG} /SAFESEH:NO")
SET(CMAKE_MODULE_LINKER_FLAGS_RELEASE "${CMAKE_MODULE_LINKER_FLAGS_RELEASE} /SAFESEH:NO")
	