cmake_minimum_required(VERSION 3.25)
project(RollDiceServer)
set(CMAKE_CXX_STANDARD 20)
set(project_name roll-dice-server)

set(SOURCES
    main.cpp
)

add_executable(dice-server ${SOURCES})

find_package(oatpp REQUIRED)

target_link_libraries(dice-server PUBLIC oatpp::oatpp)
