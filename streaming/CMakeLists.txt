cmake_minimum_required(VERSION 3.12)
project(gst_streamer)

set(CMAKE_CXX_STANDARD 14)

find_package(pybind11 REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(GST REQUIRED 
    gstreamer-1.0>=1.14
    gstreamer-app-1.0>=1.14
    gstreamer-video-1.0>=1.14
)

include_directories(${GST_INCLUDE_DIRS})
link_directories(${GST_LIBRARY_DIRS})

pybind11_add_module(gst_streamer 
    python_bindings.cpp
    gst_rtsp_streamer.cpp
)

target_link_libraries(gst_streamer PRIVATE ${GST_LIBRARIES})
target_compile_options(gst_streamer PRIVATE ${GST_CFLAGS_OTHER})

if(WIN32)
    target_compile_definitions(gst_streamer PRIVATE _WIN32_WINNT=0x0601)
endif()