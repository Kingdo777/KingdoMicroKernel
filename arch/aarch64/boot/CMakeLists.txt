function(objcopy_binary _kernel_target _binary_name)
    add_custom_target(
        ${_binary_name} ALL
        COMMAND ${CMAKE_OBJCOPY} -O binary -S $<TARGET_FILE:${_kernel_target}>
                ${_binary_name}
        DEPENDS ${_kernel_target})
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${_binary_name}
            DESTINATION ${CMAKE_INSTALL_PREFIX})
endfunction()

macro(target_sources_out_objects _target _scope _objects)
    target_sources(${_target} ${_scope} ${ARGN})

    if(NOT CMAKE_CURRENT_BINARY_DIR MATCHES "^${CMAKE_BINARY_DIR}/")
        message(
            FATAL_ERROR
                "CMAKE_CURRENT_BINARY_DIR (${CMAKE_BINARY_DIR}) must be in CMAKE_BINARY_DIR (${CMAKE_BINARY_DIR})."
        )
    endif()
    string(REGEX REPLACE "^${CMAKE_BINARY_DIR}/" "" _curr_bin_rel_path
                         ${CMAKE_CURRENT_BINARY_DIR})

    foreach(_src ${ARGN})
        if(_src MATCHES "\.(c|C)$")
            set(_obj_extension ${CMAKE_C_OUTPUT_EXTENSION})
        elseif(_src MATCHES "\.(s|S)$")
            set(_obj_extension ${CMAKE_ASM_OUTPUT_EXTENSION})
        elseif(_src MATCHES "\.(cpp|CPP|cxx|CXX|cc|CC)$")
            set(_obj_extension ${CMAKE_CXX_OUTPUT_EXTENSION})
        else()
            message(FATAL_ERROR "Unsupported file type: ${_src}")
        endif()
        list(
            APPEND
            ${_objects}
            CMakeFiles/${_target}.dir/${_curr_bin_rel_path}/${_src}${_obj_extension}
        )
    endforeach()

    unset(_obj_extension)
    unset(_curr_bin_rel_path)
endmacro()

add_subdirectory(${KMK_PLAT})
message(STATUS "init_objects: ${init_objects}")
string(REGEX REPLACE ";" "\n" init_objects "${init_objects}")

# Prepare and set linker script
configure_file(linker.tpl.ld linker.ld.S)
add_custom_target(
    linker.ld ALL
    COMMAND
        ${CMAKE_C_COMPILER} -E -x c
        -I${CMAKE_CURRENT_LIST_DIR}/${KMK_PLAT}/include
        ${CMAKE_CURRENT_BINARY_DIR}/linker.ld.S | grep -v "^#" >
        ${CMAKE_CURRENT_BINARY_DIR}/linker.ld)
add_dependencies(${kernel_target} linker.ld)
target_link_options(${kernel_target} PRIVATE -T ${CMAKE_CURRENT_BINARY_DIR}/linker.ld)