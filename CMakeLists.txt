project(virtualptu)
cmake_minimum_required(VERSION 3.18)

set (VIRT_PTU_HEADERS
    src/ptp-helpers.hpp
    src/thetacontrol.hpp
)

set (VIRT_PTU_SOURCES
    src/ptp-helpers.cpp
    src/thetacontrol.cpp
    src/virtualptu-main.cpp
)

add_executable(virtualptu ${VIRT_PTU_HEADERS} ${VIRT_PTU_SOURCES})
target_include_directories(virtualptu PRIVATE /home/tkrueger/funprojects/libptp2/src)
target_link_directories(virtualptu
PRIVATE /home/tkrueger/funprojects/libptp2/build)
target_link_libraries(virtualptu usb ptp2)
