add_subdirectory(boot)
add_subdirectory(plat/${KMK_PLAT})
add_subdirectory(mm)
add_subdirectory(machine)

target_sources(${kernel_target} PRIVATE head.S main.c tools.S)