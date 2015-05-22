set(CLANGFORMAT_PATH "NOTSET" CACHE STRING "Absolute path to the clang-format executable")

if("${CLANGFORMAT_PATH}" STREQUAL "NOTSET")
	find_program(FIND_CLANGFORMAT clang-format)
	if("${FIND_CLANGFORMAT}" STREQUAL "FIND_CLANGFORMAT-NOTFOUND")
		message(FATAL_ERROR "Could not find 'clang-format' please set CLANGFORMAT_PATH:STRING")
	else()
		set(CLANGFORMAT_PATH ${FIND_CLANGFORMAT})
		message(STATUS "Found: ${CLANGFORMAT_PATH}")
	endif()
else()
	if(NOT EXISTS ${CLANGFORMAT_PATH})
		message(WARNING "Could not find clang-format: ${CLANGFORMAT_PATH}")
	else()
		message(STATUS "Found: ${CLANGFORMAT_PATH}")
	endif()
endif()

set(FILES_TO_FORMAT)

function(Format NAME)

    add_custom_target(Format_${NAME})

    foreach(infile ${ARGN})
        get_filename_component(dir ${infile} DIRECTORY)
        if(NOT "${dir}" STREQUAL "${PROJECT_BINARY_DIR}")
            set(FILES_TO_FORMAT ${FILES_TO_FORMAT} ${infile})
        endif()
    endforeach()


    foreach(file ${FILES_TO_FORMAT})
        add_custom_command(TARGET Format_${NAME}
        COMMAND echo Processing ${file}
        COMMAND ${CLANGFORMAT_PATH} --style=file -i ${file}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
    endforeach()

endfunction(Format)