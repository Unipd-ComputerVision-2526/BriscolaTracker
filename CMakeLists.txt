cmake_minimum_required(VERSION 3.22)

project(BriscolaTraker VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
include_directories(include)

add_executable(${PROJECT_NAME}
    main.cpp
    src/utils.cpp
    src/video_manager.cpp
    src/eye.cpp
    src/reporter.cpp
    src/briscola.cpp
    )

#add_executable(test_refactoring
#    main_test.cpp
#    src/utils.cpp
#    src/video_manager.cpp
#    )

target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS})
#target_link_libraries(test_refactoring ${OpenCV_LIBS})