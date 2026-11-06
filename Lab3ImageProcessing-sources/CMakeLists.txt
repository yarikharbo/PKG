cmake_minimum_required(VERSION 3.16)

project(Lab3ImageProcessing VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Concurrent Gui Widgets)

# ---------------------------------------------------------------------------
# Model layer: pixel buffer + processing algorithms written from scratch
# (histograms, contrast/equalization, convolution filters, thresholding).
# Pure C++17 (STL only): no Qt, no third-party image processing libraries.
# ---------------------------------------------------------------------------
add_library(imagalgorithms STATIC
    model/Image8.h
    model/Image8.cpp
    model/Contrast.h
    model/Contrast.cpp
    model/Filters.h
    model/Filters.cpp
    model/Degradations.h
    model/Degradations.cpp
    model/Threshold.h
    model/Threshold.cpp
)
target_include_directories(imagalgorithms PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/model)

# ---------------------------------------------------------------------------
# ViewModel layer: background processing controller (QtConcurrent worker,
# progress and timing signals).
# ---------------------------------------------------------------------------
add_library(processingviewmodel STATIC
    viewmodel/ProcessingController.h
    viewmodel/ProcessingController.cpp
)
target_include_directories(processingviewmodel PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/viewmodel)
target_link_libraries(processingviewmodel PUBLIC imagalgorithms Qt6::Core Qt6::Concurrent Qt6::Gui)

# ---------------------------------------------------------------------------
# View layer + application entry point: Qt Widgets GUI.
# ---------------------------------------------------------------------------
add_executable(Lab3ImageProcessing WIN32
    view/main.cpp
    view/MainWindow.h
    view/MainWindow.cpp
    view/HistogramWidget.h
    view/HistogramWidget.cpp
)
target_include_directories(Lab3ImageProcessing PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/view)
target_link_libraries(Lab3ImageProcessing PRIVATE processingviewmodel Qt6::Widgets)
