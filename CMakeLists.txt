cmake_minimum_required(VERSION 3.10)
project(SDL_Image_Processing)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(SDL2 REQUIRED CONFIG COMPONENTS SDL2)
find_package(SDL2 REQUIRED CONFIG COMPONENTS SDL2main)

find_package(OpenCV REQUIRED)

if(TARGET SDL2::SDL2main)
    link_libraries(SDL2::SDL2main)
endif()

link_libraries(SDL2::SDL2 ${SDL2_IMAGE_LIBRARIES} ${OpenCV_LIBS})
include_directories(${OpenCV_INCLUDE_DIRS} ${SDL2_INCLUDE_DIRS} ${SDL2IMAGE_INCLUDE_DIRS})

add_subdirectory(src/WebcamCapture)
add_subdirectory(src/BrightestPixel)
add_subdirectory(src/FrameDifferencing)
add_subdirectory(src/CoinCollector)