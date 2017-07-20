cmake_minimum_required(VERSION 3.5)
project(obs-virtualoutput)

find_package(FFmpeg COMPONENTS avutil REQUIRED)

set(CMAKE_PREFIX_PATH "${QTDIR}")
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

include(../../cmake/external/Findlibobs.cmake)
find_package(LibObs REQUIRED)
find_package(Qt5Core REQUIRED)
find_package(Qt5Widgets REQUIRED)

set(virtualoutput_SOURCES
	virtual_output.cpp
	virtual_properties.cpp
	../queue/share_queue.cpp
	)

set(virtualoutput_HEADERS
	get_format.h
	virtual_output.h
	virtual_properties.h
	../queue/share_queue.h
	)
	

add_library(obs-virtualoutput MODULE
	${virtualoutput_SOURCES}
	${virtualoutput_HEADERS})
	
include_directories(
	"${LIBOBS_INCLUDE_DIR}/../UI/obs-frontend-api"
	${FFMPEG_INCLUDE_DIRS}
	${Qt5Core_INCLUDES}
	${Qt5Widgets_INCLUDES})
		
target_link_libraries(obs-virtualoutput
	${LIBOBS_LIBRARIES}
	${OBS_FRONTEND_LIB}
	Qt5::Core
	Qt5::Widgets)
	
