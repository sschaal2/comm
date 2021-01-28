#############################################################################
#############################################################################
#
#  This is a CMakeList.txt file originally programmed for the CLMC/AMD labs
#  at the University of Southern California and the Max-Planck-Institute for
#  Intelligent Systems. We use a mixutre of explicit makefiles and cmake, but 
#  primarily we relay on cmake for all major compile dependencies. All our
#  software is provided under a slightly modified version of the LGPL license
#  to be found at http://www-clmc.usc.edu/software/license.
#
#  Copyright by Stefan Schaal, 2014
#
#############################################################################
#############################################################################
# which version are we using

cmake_minimum_required(VERSION 2.8)

#############################################################################
# include common cmake components

include($ENV{LAB_ROOT}/config/cmake/LAB.cmake)

#############################################################################
# user defined cmake components

# set global compile type
set(CMAKE_BUILD_TYPE RelWithDebInfo) # Optimization with debugging info
#set(CMAKE_BUILD_TYPE Release)       # Optimization 
#set(CMAKE_BUILD_TYPE Debug)         # Debug

# local defines
include_directories(BEFORE ../src)
include_directories(BEFORE ../include)

set(SOURCES
	udp_communication.cpp
	udp_main.cpp )

set(HEADERS
	../include/udp_communication.h
	../include/serial_communication.h )	

add_library(comm UDP_communication.cpp serial_communication.cpp)
install(FILES ${HEADERS} DESTINATION ${LAB_INCLUDES})
install(TARGETS comm ARCHIVE DESTINATION ${LAB_LIBDIR})

add_executable(xudpTest ${SOURCES})
target_link_libraries(xudpTest comm ${LAB_STD_LIBS})
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/xudpTest DESTINATION ${LAB_BINDIR})
