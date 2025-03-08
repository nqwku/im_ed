cmake_minimum_required(VERSION 3.30)
project(img_ed C)

set(CMAKE_C_STANDARD 17)

find_package(OpenMP REQUIRED)
if(OpenMP_C_FOUND)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mavx2 ${OpenMP_C_FLAGS}")
endif()

add_executable(img_ed
        src/main.c
        include/image_utils.h
        include/stb_include.h
        src/file_utils.c
        src/string_utils.c
        src/filter.c
        include/logger.h
        src/logger.c)

include_directories(include)

target_link_libraries(img_ed PRIVATE m)
if(OpenMP_C_FOUND)
    target_link_libraries(img_ed PRIVATE OpenMP::OpenMP_C)
endif()