cmake_minimum_required(VERSION 3.22)

project(BriscolaTraker VERSION 1.0 LANGUAGES CXX)

include_directories(include)

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(include)

add_executable(${PROJECT_NAME}
    main.cpp
    src/utils.cpp
    src/eye.cpp
    )

target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS})