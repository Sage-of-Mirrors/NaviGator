cmake_minimum_required(VERSION 3.12)
project(navigator)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(GLFW3 REQUIRED)

add_subdirectory(lib)

file(GLOB NAVIGATOR_SRC
    # NaviGator
    "src/*.cpp"
    "include/*.hpp"
    
    "src/application/*.cpp"
    "include/application/*.hpp"
    
    "src/ui/*.cpp"
    "include/ui/*.hpp"
    
    # bStream
    "lib/bStream/*.cpp"
    
    # glad
    "lib/glad/src/*.c"

    # ImGuiFileDialog
    "lib/ImGuiFileDialog/ImGuiFileDialog.cpp"
)

add_executable(navigator ${NAVIGATOR_SRC})

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/asset/font/MaterialSymbolsRounded.ttf DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/asset/font)

target_include_directories(navigator PUBLIC include lib/glad/include lib/bStream lib/glm lib/ImGuiFileDialog)
target_link_libraries(navigator PUBLIC glm imgui glfw)
