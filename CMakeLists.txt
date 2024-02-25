cmake_minimum_required(VERSION 3.12)
project(navigator)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(GLFW3 REQUIRED)

add_subdirectory(lib)

file(GLOB NAVIGATOR_SRC
    # NaviGator
    "src/*.cpp"
    "include/*.hpp"
    "include/*.h"
    
    "src/application/*.cpp"
    "include/application/*.hpp"
    
    "src/ui/*.cpp"
    "include/ui/*.hpp"

    "src/util/*.cpp"
    "include/util/*.hpp"

    "src/ubo/*.cpp"
    "include/ubo/*.hpp"

    "src/tracks/*.cpp"
    "include/tracks/*.hpp"

    "include/primitives/*.hpp"
    
    # glad
    "lib/glad/src/*.c"

    # ImGuiFileDialog
    "lib/ImGuiFileDialog/ImGuiFileDialog.cpp"
)

add_executable(navigator ${NAVIGATOR_SRC})

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/asset DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

target_include_directories(navigator PUBLIC include lib/glad/include lib/glm lib/ImGuiFileDialog lib/librdr3/include lib/Recast/include lib/pugixml/src)
target_link_libraries(navigator PUBLIC glm imgui glfw librdr3 Recast pugixml)
