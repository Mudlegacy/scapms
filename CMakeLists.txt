cmake_minimum_required(VERSION 3.16)
project(SCAPMS LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)          # runs Qt's code generator (moc) automatically

find_package(Qt6 REQUIRED COMPONENTS Widgets)

# 1. The core: model + controller + utils. It contains NO Qt code.
file(GLOB CORE_SOURCES CONFIGURE_DEPENDS
    model/*.cpp
    controller/*.cpp
    utils/*.cpp
)
add_library(scapms_core STATIC ${CORE_SOURCES})
target_include_directories(scapms_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# 2. The console test program (our old main)
add_executable(scapms_console console_demo.cpp)
target_link_libraries(scapms_console PRIVATE scapms_core)

# 3. The GUI application
file(GLOB GUI_SOURCES CONFIGURE_DEPENDS gui/*.cpp)
add_executable(scapms main.cpp ${GUI_SOURCES})
target_link_libraries(scapms PRIVATE scapms_core Qt6::Widgets)
# 4. A test for the reminder logic
add_executable(scapms_reminder_test tests/reminder_test.cpp)
target_link_libraries(scapms_reminder_test PRIVATE scapms_core)