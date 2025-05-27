cmake_minimum_required(VERSION 3.14)

# 设置 Toolchain
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/toolchain.cmake)
    message(FATAL_ERROR "Toolchain file not found: ${CMAKE_CURRENT_SOURCE_DIR}/toolchain.cmake")
endif()
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/toolchain.cmake)

# 生成 compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# Enable VERBOSE，生成详细的编译信息
set(CMAKE_VERBOSE_MAKEFILE OFF)

# KingdoMicroKernel a.k.a. KMK
project(KingdoMicroKernel C ASM) 

# Set Default Architecture
if(NOT DEFINED KMK_ARCH)
    message(STATUS "KMK_ARCH not defined, defaulting to aarch64")
    set(KMK_ARCH "aarch64")
endif()

if(NOT DEFINED KMK_PLAT)
    message(STATUS "KMK_PLAT not defined, defaulting to raspi3")
    set(KMK_PLAT "raspi3")
endif()

# Create kernel.img target
set(kernel_target "kernel.img")
add_executable(${kernel_target})
install(TARGETS ${kernel_target} DESTINATION ${CMAKE_INSTALL_PREFIX})

# Set warning level
list(APPEND _compile_options -Wall) # 添加编译器选项, 禁用所有警告
list(APPEND _compile_options -Werror) # 将所有警告视为错误
list(APPEND _compile_options -Wno-unused-variable) # 忽略未使用的变量警告
list(APPEND _compile_options -Wno-unused-function) # 忽略未使用的函数警告

# Generic compile settings
list(APPEND _compile_options -nostdinc) # 禁用标准头文件搜索路径
list(APPEND _compile_options -ffreestanding) # 告诉编译器代码运行在裸机环境中，禁用标准库依赖，适用于 ​​内核、Bootloader、嵌入式固件​​ 等场景
list(APPEND _c_compile_options) # C编译器优化选项，设置为-O0，表示不进行优化并启用调试信息
list(APPEND _asm_compile_options) # 汇编器编译选项，当前为空
list(APPEND _compile_definitions) # 定义编译器宏, 当前为空
list(APPEND _c_compile_definitions) # C编译器宏, 当前为空
list(APPEND _asm_compile_definitions __ASM__) # gcc -D__ASM__，用于当前文件是被汇编代码包含还是被C代码包含

# Set compile settings to target
target_compile_definitions(${kernel_target} PRIVATE ${_compile_definitions})
target_compile_definitions(${kernel_target} PRIVATE $<$<COMPILE_LANGUAGE:ASM>:${_asm_compile_definitions}>)
target_compile_definitions(${kernel_target} PRIVATE $<$<COMPILE_LANGUAGE:C>:${_c_compile_definitions}>)
target_compile_options(${kernel_target} PRIVATE ${_compile_options})
target_compile_options(${kernel_target} PRIVATE $<$<COMPILE_LANGUAGE:ASM>:${_asm_compile_options}>)
target_compile_options(${kernel_target} PRIVATE $<$<COMPILE_LANGUAGE:C>:${_c_compile_options}>)

# Linker options
target_link_options(${kernel_target} PRIVATE -no-pie -nostdlib -nostartfiles -Wl,--build-id=none)

# set Log Level by build type
target_compile_definitions(${kernel_target} PRIVATE $<$<CONFIG:Debug>:LOG_LEVEL=2>)
target_compile_definitions(${kernel_target} PRIVATE $<$<CONFIG:Release>:LOG_LEVEL=1>)

# Include directories
target_include_directories(${kernel_target} PRIVATE include)
target_include_directories(${kernel_target} PRIVATE include/arch/${KMK_ARCH})
target_include_directories(${kernel_target} PRIVATE include/arch/${KMK_ARCH}/plat/${KMK_PLAT})

# Add source code directories
add_subdirectory(arch/${KMK_ARCH})
add_subdirectory(lib)
add_subdirectory(mm)
add_subdirectory(syscall)