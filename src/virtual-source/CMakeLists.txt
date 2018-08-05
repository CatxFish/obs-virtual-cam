cmake_minimum_required(VERSION 3.5)
project(obs-virtualsource)

find_package(FFmpeg COMPONENTS swscale REQUIRED)

set(virtualsource_SOURCES
	dllmain.cpp
	virtual-cam.cpp
	virtual-audio.cpp
	virtua-source.def
	../queue/share_queue_read.cpp
	)

set(virtualsource_HEADERS
	virtual-cam.h
	virtual-audio.h
	../queue/share_queue_read.h
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
	
SET(DSHOW_LIBRARY optimized ${DShowLib} debug ${DShowLibD})	
		
target_link_libraries(obs-virtualsource
	${FFMPEG_LIBRARIES}
	${DSHOW_LIBRARY}
	winmm
	Strmiids
	)
	
if(MSVC)
	set(CMAKE_EXE_LINKER_FLAGS
      "${CMAKE_EXE_LINKER_FLAGS} /DEF:virtua-source.def /NODEFAULTLIB")
	
    foreach(var CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
      CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
      if(${var} MATCHES "/MD")
        string(REGEX REPLACE "/MD" "/MT" ${var} "${${var}}")
      endif()
    endforeach()	
		  
endif()
SET(CMAKE_MODULE_LINKER_FLAGS_DEBUG "${CMAKE_MODULE_LINKER_FLAGS_DEBUG} /SAFESEH:NO")
SET(CMAKE_MODULE_LINKER_FLAGS_RELEASE "${CMAKE_MODULE_LINKER_FLAGS_RELEASE} /SAFESEH:NO")
	