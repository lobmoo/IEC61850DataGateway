cmake_minimum_required(VERSION 3.16)

project(goose_subpub_exec  LANGUAGES C)

# 查找目录下的所有源文件
# 假设您希望在查找源文件时排除 CMakeFiles 目录
file(GLOB PUB_EXAMPLE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/goose_publisher_example.c")
file(GLOB SUB_EXAMPLE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/goose_subscriber_example.c")

message(STATUS "Collected CXX sources: ${EXAMPLE_SOURCES}")


# 添加可执行文件目标
add_executable(goose_pub_exec ${PUB_EXAMPLE_SOURCES})
add_executable(goose_sub_exec ${SUB_EXAMPLE_SOURCES})
# set_target_properties(${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set_target_properties(goose_pub_exec PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set_target_properties(goose_sub_exec PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

#头文件
set(OBJ_CPP_INCLUDE
    ${CMAKE_CURRENT_SOURCE_DIR}/goose_subpub
)

list(APPEND OBJ_CPP_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/iec61850/${ARCH}/include)

target_include_directories(goose_pub_exec PRIVATE  ${OBJ_CPP_INCLUDE})
target_include_directories(goose_sub_exec PRIVATE  ${OBJ_CPP_INCLUDE})



    
target_link_libraries(goose_pub_exec PRIVATE
    ${OBJ_CPP_LIBS}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/iec61850/${ARCH}/lib/libiec61850.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/iec61850/${ARCH}/lib/libhal.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/iec61850/${ARCH}/lib/libhal-shared.a
    pthread
    rt
)
                                                                       

target_link_libraries(goose_sub_exec PRIVATE 
    ${OBJ_CPP_LIBS}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/iec61850/${ARCH}/lib/libiec61850.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/iec61850/${ARCH}/lib/libhal.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/iec61850/${ARCH}/lib/libhal-shared.a
    pthread
    rt
)

target_compile_options(goose_pub_exec  PRIVATE -Wall -Wextra -g)
target_compile_options(goose_sub_exec  PRIVATE -Wall -Wextra -g)


