cmake_minimum_required(VERSION 3.10)
project(CLI_Framework C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 默认平台为 x86 (PC)
set(CLI_PLATFORM "x86" CACHE STRING "Target platform: x86, stm32f1, etc.")

# 源文件列表
set(SOURCES
    src/cli.c
)

# 根据平台选择对应的端口文件
if(CLI_PLATFORM STREQUAL "x86")
    list(APPEND SOURCES demo/cli_demo_port_x86.c)
elseif(CLI_PLATFORM STREQUAL "stm32f1")
    list(APPEND SOURCES demo/cli_demo_port_stm32f1.c)
    # 可以添加针对 STM32 的编译选项，如 -mcpu=cortex-m3 等
else()
    message(FATAL_ERROR "Unsupported platform: ${CLI_PLATFORM}")
endif()
# 演示文件
list(APPEND SOURCES demo/cli_demo.c)
list(APPEND SOURCES demo/cli_demo_commands.c)

# 头文件目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)

# 可执行文件
add_executable(cli_demo ${SOURCES})

# 如果是 Windows 且使用 MinGW，可能需要链接某些库
if(WIN32)
    target_link_libraries(cli_demo PRIVATE mingwex)
endif()

# 设置输出目录
set_target_properties(cli_demo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# 编译选项
if(CMAKE_COMPILER_IS_GNUCC)
    target_compile_options(cli_demo PRIVATE -Wall -Wextra -Wpedantic)
endif()
