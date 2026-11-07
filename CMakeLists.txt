cmake_minimum_required(VERSION 3.21)
project(CarePilot VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    main.cpp model/*.cpp controller/*.cpp gui/*.cpp utils/*.cpp)
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS
    model/*.h controller/*.h gui/*.h utils/*.h)

add_executable(CarePilot ${SOURCES} ${HEADERS})
target_include_directories(CarePilot PRIVATE ${CMAKE_SOURCE_DIR})
target_link_libraries(CarePilot PRIVATE Qt6::Widgets)

add_custom_command(TARGET CarePilot POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/data $<TARGET_FILE_DIR:CarePilot>/data)

set_target_properties(CarePilot PROPERTIES WIN32_EXECUTABLE ON)