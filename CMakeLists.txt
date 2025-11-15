cmake_minimum_required(VERSION 4.0)
project(FAAL)

set(CMAKE_CXX_STANDARD 20)

find_package(PkgConfig REQUIRED)

pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
include_directories(${GTK3_INCLUDE_DIRS})
link_directories(${GTK3_LIBRARY_DIRS})
add_definitions(${GTK3_CFLAGS_OTHER})

pkg_check_modules(LAYER REQUIRED gtk-layer-shell-0)
include_directories(${LAYER_INCLUDE_DIRS})
link_directories(${LAYER_LIBRARY_DIRS})
add_definitions(${LAYER_CFLAGS_OTHER})

add_executable(FAAL main.cpp)

target_link_libraries(FAAL ${GTK3_LIBRARIES} ${LAYER_LIBRARIES})