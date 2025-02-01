cmake_minimum_required(VERSION 3.30)
project(img_ed C)

set(CMAKE_C_STANDARD 17)

add_executable(img_ed
        src/main.c)

target_link_libraries(img_ed PRIVATE m)

include_directories(include)