cmake_minimum_required(VERSION 2.8)
project(VideoStab)

set( CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules 
    )

include_directories( ${CMAKE_SOURCE_DIR}/external/tclap-1.2.1/include/ )

add_definitions( -std=c++11 -Wall )

set(CMAKE_BUILD_TYPE Debug )

find_package( OPENCV REQUIRED 
    # COMPONENTS 
    # core highgui imgproc calib3d 
    # video
    # # This is required for opencv3.
    # videoio
    )
# Either use LIBTIFF or TINYTIFF.
find_package( TIFF REQUIRED )
add_definitions( -DUSE_LIBTIFF )


add_executable(videostab 
    src/main.cpp
    src/videoio.cpp
    )

target_link_libraries( videostab  
    ${TIFF_LIBRARIES}
    ${OPENCV_LIBRARIES}
    )

install( TARGETS videostab 
    RUNTIME
    DESTINATION bin
    )

