cmake_minimum_required(VERSION 3.16)
project(sdv-qt VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Charts)

# libpcap (Linux: apt install libpcap-dev) / Npcap SDK (Windows)
# Windows: folder SDK bisa diatur dengan -DPCAP_ROOT="C:/Program Files/Npcap/npcap-sdk-1.16"
set(PCAP_ROOT "" CACHE PATH "Folder Npcap SDK (hanya Windows)")
set(_pcap_hints
    "${PCAP_ROOT}" "$ENV{PCAP_ROOT}"
    "C:/Program Files/Npcap/npcap-sdk-1.16"
    "C:/Program Files/Npcap SDK"
    "C:/Npcap-SDK")

find_path(PCAP_INCLUDE_DIR pcap/pcap.h HINTS ${_pcap_hints} PATH_SUFFIXES Include include)
find_library(PCAP_LIBRARY NAMES pcap wpcap HINTS ${_pcap_hints} PATH_SUFFIXES Lib/x64 Lib lib)
if(NOT PCAP_INCLUDE_DIR OR NOT PCAP_LIBRARY)
    message(FATAL_ERROR
        "libpcap / Npcap SDK tidak ditemukan.\n"
        "  include: ${PCAP_INCLUDE_DIR}\n  library: ${PCAP_LIBRARY}\n"
        "Set -DPCAP_ROOT=<folder SDK yang berisi Include/ dan Lib/>")
endif()
message(STATUS "pcap include: ${PCAP_INCLUDE_DIR}")
message(STATUS "pcap library: ${PCAP_LIBRARY}")

add_executable(sdv-qt WIN32
    src/main.cpp
    src/ZcuTelemetry.h
    src/Params.h
    src/AppState.h
    src/AppState.cpp
    src/RawEthMonitor.h
    src/RawEthMonitor.cpp
    src/SetupWindow.h
    src/SetupWindow.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/DataListPage.h
    src/DataListPage.cpp
    src/GraphPage.h
    src/GraphPage.cpp
    src/DataLoggingPage.h
    src/DataLoggingPage.cpp
)

target_include_directories(sdv-qt PRIVATE ${PCAP_INCLUDE_DIR} src)
target_link_libraries(sdv-qt PRIVATE Qt6::Widgets Qt6::Charts ${PCAP_LIBRARY})
