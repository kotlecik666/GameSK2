cmake_minimum_required(VERSION 3.5)
project(GameSK2)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11 -pthread")

#set(SOURCE_FILES main.cpp server.cpp)
add_executable(Client main.cpp GameStructs.h)
add_executable(Server server.cpp GameStructs.h)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake_modules")
find_package(SFML REQUIRED system window graphics network audio)
if (SFML_FOUND)
    include_directories(${SFML_INCLUDE_DIR})
    target_link_libraries(Client ${SFML_LIBRARIES})
endif()