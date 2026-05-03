cmake_minimum_required(VERSION 3.25)
project(RollDiceServer)
set(CMAKE_CXX_STANDARD 20)
set(project_name roll-dice-server)

set(SOURCES
    main.cpp
)

find_package(opentelemetry-cpp CONFIG REQUIRED)

add_executable(dice-server ${SOURCES})

find_package(oatpp REQUIRED)
target_link_libraries(dice-server PUBLIC oatpp::oatpp)

find_package(spdlog REQUIRED)
target_link_libraries(dice-server PRIVATE spdlog::spdlog)

target_include_directories(dice-server PRIVATE ${OPENTELEMETRY_CPP_INCLUDE_DIRS})
target_link_libraries(dice-server PRIVATE ${OPENTELEMETRY_CPP_LIBRARIES})

include_directories(../third_party/argparse/include)
