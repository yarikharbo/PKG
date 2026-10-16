cmake_minimum_required(VERSION 3.16)

project(Lab2ImageInfo VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Concurrent Widgets)

# ---------------------------------------------------------------------------
# Model layer: byte-level header parsers for BMP/PNG/GIF/JPEG/TIFF/PCX.
# Pure C++17 (STL only): no Qt, no GUI, no third-party metadata libraries.
# ---------------------------------------------------------------------------
add_library(imagecore STATIC
    model/ByteReader.h
    model/ByteReader.cpp
    model/ImageInfo.h
    model/ImageInfo.cpp
    model/ImageInfoReader.h
    model/ImageInfoReader.cpp
    model/BmpParser.h
    model/BmpParser.cpp
    model/PngParser.h
    model/PngParser.cpp
    model/GifParser.h
    model/GifParser.cpp
    model/JpegParser.h
    model/JpegParser.cpp
    model/TiffParser.h
    model/TiffParser.cpp
    model/PcxParser.h
    model/PcxParser.cpp
)
target_include_directories(imagecore PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/model)

# ---------------------------------------------------------------------------
# ViewModel layer: multithreaded folder scanner (QtConcurrent) with progress
# and batched row delivery to keep the UI responsive.
# ---------------------------------------------------------------------------
add_library(scannerviewmodel STATIC
    viewmodel/FolderScanner.h
    viewmodel/FolderScanner.cpp
)
target_include_directories(scannerviewmodel PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/viewmodel)
target_link_libraries(scannerviewmodel PUBLIC imagecore Qt6::Core Qt6::Concurrent)

# ---------------------------------------------------------------------------
# View layer + application entry point: Qt Widgets.
# ---------------------------------------------------------------------------
add_executable(Lab2ImageInfo WIN32
    view/main.cpp
    view/MainWindow.h
    view/MainWindow.cpp
)
target_include_directories(Lab2ImageInfo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/view)
target_link_libraries(Lab2ImageInfo PRIVATE scannerviewmodel Qt6::Widgets)
