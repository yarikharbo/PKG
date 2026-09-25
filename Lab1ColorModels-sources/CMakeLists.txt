cmake_minimum_required(VERSION 3.16)

project(Lab1ColorModels VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

# ---------------------------------------------------------------------------
# Model layer: pure color math. No Qt, no GUI, no I/O.
# ---------------------------------------------------------------------------
add_library(colormath STATIC
    model/ColorConverter.h
    model/ColorConverter.cpp
)
target_include_directories(colormath PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/model)

# ---------------------------------------------------------------------------
# ViewModel layer: Qt Core glue between View and Model.
# ---------------------------------------------------------------------------
add_library(colorviewmodel STATIC
    viewmodel/ColorViewModel.h
    viewmodel/ColorViewModel.cpp
)
target_include_directories(colorviewmodel PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/viewmodel)
target_link_libraries(colorviewmodel PUBLIC colormath Qt6::Core)

# ---------------------------------------------------------------------------
# View layer + application entry point: Qt Widgets.
# ---------------------------------------------------------------------------
add_executable(Lab1ColorModels WIN32
    view/main.cpp
    view/MainWindow.h
    view/MainWindow.cpp
    view/ModelGroupWidget.h
    view/ModelGroupWidget.cpp
)
target_include_directories(Lab1ColorModels PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/view)
target_link_libraries(Lab1ColorModels PRIVATE colorviewmodel Qt6::Widgets)
