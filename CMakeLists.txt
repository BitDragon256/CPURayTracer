cmake_minimum_required(VERSION 3.25)

project(RayTracingTest)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(SDL_CMAKE_DEBUG_POSTFIX "d" 
CACHE STRING "Name suffix for debug builds") 

set(SDL2_DIR "C:/Program Files/vcpkg/packages/sdl2_x64-windows/share/sdl2")
find_package(SDL2 REQUIRED)

set(glm_DIR "C:/Program Files/vcpkg/packages/glm_x64-windows/share/glm")
find_package(glm CONFIG REQUIRED)

find_package(OpenMP REQUIRED)

add_executable(RayTracingTest main.cpp ray_tracing_lib.cpp ray_tracing_lib.h)
target_link_libraries(RayTracingTest PRIVATE SDL2::SDL2 SDL2::SDL2main glm::glm OpenMP::OpenMP_CXX)

if (WIN32)
    add_custom_command(
        TARGET RayTracingTest POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:SDL2::SDL2>" "$<TARGET_FILE_DIR:RayTracingTest>"
        VERBATIM
    )
endif()
