cmake_minimum_required(VERSION 3.16)

project(demo_61850_exec  LANGUAGES  C)

set(CMAKE_CXX_STANDARD 17) 
# set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_COMPILER g++)


# 查找目录下的所有源文件
# 假设您希望在查找源文件时排除 CMakeFiles 目录
file(GLOB EXAMPLE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")


message(STATUS "Collected CXX sources: ${EXAMPLE_SOURCES}")


# 添加可执行文件目标
add_executable(${PROJECT_NAME} ${EXAMPLE_SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

#头文件
set(OBJ_CPP_INCLUDE
    ${CMAKE_CURRENT_SOURCE_DIR}/
)

list(APPEND OBJ_CPP_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/iec61850/${ARCH}/include)

target_include_directories(${PROJECT_NAME} PRIVATE  ${OBJ_CPP_INCLUDE})

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${OBJ_CPP_LIBS}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/iec61850/${ARCH}/lib/libiec61850.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/iec61850/${ARCH}/lib/libhal.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/iec61850/${ARCH}/lib/libhal-shared.a
    pthread
    rt
    m
)
                                                                       

target_compile_options(${PROJECT_NAME}  PRIVATE -Wall -Wextra -g)


