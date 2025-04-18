# CMake toolchain file for cross-compiling MKM

set(MKM_CROSS_COMPILE "aarch64-linux-gnu-")

# Set toolchain executables
set(CMAKE_ASM_COMPILER "${MKM_CROSS_COMPILE}gcc")
set(CMAKE_C_COMPILER "${MKM_CROSS_COMPILE}gcc")
# set(CMAKE_CXX_COMPILER "${MKM_CROSS_COMPILE}g++")
set(CMAKE_AR "${MKM_CROSS_COMPILE}ar")
set(CMAKE_NM "${MKM_CROSS_COMPILE}nm")
set(CMAKE_OBJCOPY "${MKM_CROSS_COMPILE}objcopy")
set(CMAKE_OBJDUMP "${MKM_CROSS_COMPILE}objdump")
set(CMAKE_RANLIB "${MKM_CROSS_COMPILE}ranlib")
set(CMAKE_STRIP "${MKM_CROSS_COMPILE}strip")

# Set build type
if(MKM_KERNEL_DEBUG)
    set(CMAKE_BUILD_TYPE "Debug")
else()
    set(CMAKE_BUILD_TYPE "Release")
endif()

# include(${CMAKE_CURRENT_LIST_DIR}/_common.cmake)

# Set the target system (automatically set CMAKE_CROSSCOMPILING to true)
set(CMAKE_SYSTEM_NAME "Generic")
set(CMAKE_SYSTEM_PROCESSOR ${MKM_ARCH})