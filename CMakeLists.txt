project(virtualptu)
cmake_minimum_required(VERSION 3.18)

find_package(PkgConfig) 
#find gstreamer (maybe this can be more elegant) maybe re-use FindGStreamer.cmake from google
pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(GSTREAMER REQUIRED gstreamer-1.0)
pkg_check_modules(GSTREAMER-APP REQUIRED gstreamer-app-1.0)
add_definitions(-DGST_API_VERSION_1=1)

#TODO
#findpackage libptp2
#findpackage libusb
#find_package(libuvc REQUIRED)
#find package thetacontrol
#find package TCLAP

set (VIRT_PTU_HEADERS
  )

set (VIRT_PTU_SOURCES
    src/virtualptu-main.cpp
)

#basically a temp step to deal with C and C++ combo and speedup
add_library(thetauvc  src/thetauvc.h  src/thetauvc.c)

add_executable(virtualptu ${VIRT_PTU_HEADERS} ${VIRT_PTU_SOURCES})
target_include_directories(virtualptu
    #PRIVATE /home/tkrueger/projects/thetaV/libptp2/src
    PRIVATE ${GSTREAMER_INCLUDE_DIRS}
)

#target_link_directories(virtualptu PRIVATE /home/tkrueger/projects/thetaV/libptp2/build)
target_link_libraries(virtualptu
    thetacontrol    
    thetauvc    
    pthread
    ${GSTREAMER_LIBRARIES}
    ${GSTREAMER-APP_LIBRARIES}
    uvc
    usb
    ptp2    
)
