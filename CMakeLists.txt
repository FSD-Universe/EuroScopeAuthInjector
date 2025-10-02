cmake_minimum_required(VERSION 3.30)
project(EuroScopeInjector LANGUAGES CXX VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_GENERATOR_PLATFORM "Win32")
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

find_package(CURL REQUIRED)

message("Build Type: ${CMAKE_BUILD_TYPE} ${CMAKE_CXX_FLAGS}")
message("PROJECT_NAME: ${PROJECT_NAME}")
message("PROJECT_VERSION: ${PROJECT_VERSION}")
message("PROJECT_SOURCE_DIR: ${PROJECT_SOURCE_DIR}")
message("PROJECT_BINARY_DIR: ${PROJECT_BINARY_DIR}")

include_directories(include src)
link_directories("${PROJECT_SOURCE_DIR}/lib")

add_library(${PROJECT_NAME} SHARED main.cpp src/AuthInjector.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE EuroScopePlugIn detours CURL::libcurl)
