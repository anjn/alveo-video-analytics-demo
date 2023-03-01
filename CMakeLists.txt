cmake_minimum_required(VERSION 3.2)

project(alveo-video-analytics-demo)

include(FetchContent)
include(CMakePrintHelpers)

FetchContent_Declare(
    zeromq
    SOURCE_DIR ${PROJECT_SOURCE_DIR}/external/zeromq-4.3.2
)

FetchContent_Declare(
    msgpack
    SOURCE_DIR ${PROJECT_SOURCE_DIR}/external/msgpack-3.2.0
)

FetchContent_Declare(
    bytetrack
    SOURCE_DIR ${PROJECT_SOURCE_DIR}/external/ByteTrack-cpp-main
)

set(MSGPACK_BUILD_TESTS OFF CACHE INTERNAL "")
set(BUILD_BYTETRACK_TEST OFF CACHE INTERNAL "")
set(ZMQ_BUILD_TESTS OFF CACHE INTERNAL "")

FetchContent_MakeAvailable(zeromq msgpack bytetrack)

find_package(PkgConfig REQUIRED)
pkg_check_modules(
    gstreamer REQUIRED IMPORTED_TARGET
    gstreamer-1.0
    gstreamer-app-1.0
    gstreamer-allocators-1.0
    gstreamer-video-1.0
    gstreamer-rtsp-1.0
    gstreamer-rtsp-server-1.0
)
pkg_check_modules(
    ffmpeg REQUIRED IMPORTED_TARGET
    libavcodec
    libavformat
    libavutil
    libswresample
    libswscale
)
pkg_search_module(
    xrt REQUIRED IMPORTED_TARGET
    xrt
)

find_package(OpenCV REQUIRED)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a")

include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR}/external/arg-master)
include_directories(${PROJECT_SOURCE_DIR}/external/cppzmq-4.4.1)
include_directories(${PROJECT_SOURCE_DIR}/external/toml11-3.7.1)
include_directories(${PROJECT_SOURCE_DIR}/src)

function(add_target NAME DIR)
    add_executable(${NAME} ${DIR}/${NAME}.cpp)
    target_link_libraries(${NAME}
        libzmq-static
        msgpackc-static
        bytetrack-static
        PkgConfig::gstreamer
        PkgConfig::ffmpeg
        PkgConfig::xrt
        ${OpenCV_LIBS}
        -lvitis_ai_library-yolov3
        -lvitis_ai_library-classification
        -lvitis_ai_library-vehicleclassification
        -lvitis_ai_library-dpu_task
        -lvitis_ai_library-xnnpp
        -lvitis_ai_library-model_config
        -lvitis_ai_library-math
        -lvart-runner
        -lvart-util
        -lxir
        -ldl
        -pthread
    )
endfunction()

foreach(TARGET ml_server video_server)
    add_target(${TARGET} src)
endforeach()

foreach(TEST
        test_yolov3 test_yolov3_batch test_yolov3_server test_bcc_server
        test_gst_pipeline_utils test_rtsp_server test_rtsp_repeat test_rtspclientsink
)
    add_target(${TEST} test)
endforeach()
