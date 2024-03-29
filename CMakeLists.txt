# Minimum required CMake version
cmake_minimum_required(VERSION 3.20.2)

# Project name
project(meeting_sdk_linux_sample)

# Your existing project configuration goes here
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_COMPILER /usr/bin/g++)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(ZOOM_SDK lib/zoomsdk)

find_package(ada REQUIRED)
find_package(CLI11 REQUIRED)
find_path(JWT_CPP_INCLUDE_DIRS "jwt-cpp/base.h")

# Find Poco
find_package(Poco REQUIRED COMPONENTS Foundation NetSSL Crypto Net Util XML JSON Crypto)

find_package(PkgConfig REQUIRED)
pkg_check_modules(deps REQUIRED IMPORTED_TARGET glib-2.0)

include_directories(${ZOOM_SDK}/h)
link_directories(${ZOOM_SDK} ${ZOOM_SDK})
link_directories(${ZOOM_SDK} ${ZOOM_SDK}/qt_libs)
link_directories(${ZOOM_SDK} ${ZOOM_SDK}/qt_libs/Qt/lib)

include_directories(${Poco_INCLUDE_DIRS})

add_executable(zoomsdk
    src/main.cpp
    src/Zoom.cpp
    src/Zoom.h
    src/raw-stream/DeepgramWSHelper.cpp  
    src/raw-stream/DeepgramWSHelper.h
    src/raw-stream/DeepgramJsonParser.cpp  
    src/raw-stream/DeepgramJsonParser.h     
    src/Config.cpp
    src/Config.h
    src/util/Singleton.h
    src/util/Log.h
    src/events/AuthServiceEvent.cpp
    src/events/AuthServiceEvent.h
    src/events/MeetingServiceEvent.cpp
    src/events/MeetingServiceEvent.h
    src/events/MeetingReminderEvent.cpp
    src/events/MeetingReminderEvent.h
    src/events/MeetingRecordingCtrlEvent.cpp
    src/events/MeetingRecordingCtrlEvent.h
    src/raw-stream/ZoomSDKAudioRawDataDelegate.cpp
    src/raw-stream/ZoomSDKAudioRawDataDelegate.h
    src/raw-stream/ZoomSDKRendererDelegate.cpp
    src/raw-stream/ZoomSDKRendererDelegate.h
)

target_include_directories(zoomsdk PRIVATE ${Poco_INCLUDE_DIRS})
target_link_libraries(zoomsdk PRIVATE meetingsdk Poco::Foundation Poco::NetSSL Poco::Crypto Poco::Net ada::ada CLI11::CLI11 PkgConfig::deps)
