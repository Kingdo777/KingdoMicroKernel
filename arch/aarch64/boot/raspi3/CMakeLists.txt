target_include_directories(${kernel_target} PRIVATE include)

list(APPEND _init_sources init/start.S
                          init/tools.S
                          init/init.c
                          init/mmu.c
                          peripherals/uart.c)

target_sources_out_objects(${kernel_target} PRIVATE init_objects ${_init_sources})

set(init_objects ${init_objects} PARENT_SCOPE)

objcopy_binary(${kernel_target} kernel8.img)
