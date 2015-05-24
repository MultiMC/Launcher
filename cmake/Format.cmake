find_program(CLANGFORMAT_PATH clang-format)

add_custom_target(format)

function(Format NAME)
    if(EXISTS ${CLANGFORMAT_PATH})
        add_custom_target(format-${NAME})

        set(FILES_TO_FORMAT)

        foreach(infile ${ARGN})
            get_filename_component(dir ${infile} DIRECTORY)
            if(NOT "${dir}" STREQUAL "${PROJECT_BINARY_DIR}")
                set(FILES_TO_FORMAT ${FILES_TO_FORMAT} ${infile})
            endif()
        endforeach()


        foreach(file ${FILES_TO_FORMAT})
            add_custom_command(TARGET format-${NAME}
            COMMAND echo Processing ${file}
            COMMAND ${CLANGFORMAT_PATH} --style=file -i ${file}
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
        endforeach()
    endif()
    
    add_dependencies(format format-${NAME})

endfunction(Format)
