find_package(Qt6 REQUIRED COMPONENTS Test)

set(TEST_RESOURCE_PATH ${CMAKE_CURRENT_LIST_DIR})

message(${TEST_RESOURCE_PATH})

function(add_unit_test name)
    set(options "")
    set(oneValueArgs DATA)
    set(multiValueArgs SOURCES LIBS)
    set(test_name "${name}_test")

    cmake_parse_arguments(OPT "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    if(WIN32)
        add_executable(${test_name} ${OPT_SOURCES} ${TEST_RESOURCE_PATH}/UnitTest/test.rc)
    else()
        add_executable(${test_name} ${OPT_SOURCES})
    endif()

    if(NOT "${OPT_DATA}" STREQUAL "")
        set(TEST_DATA_PATH "${CMAKE_CURRENT_BINARY_DIR}/data")
        set(TEST_DATA_PATH_SRC "${CMAKE_CURRENT_SOURCE_DIR}/${OPT_DATA}")
        message("From ${TEST_DATA_PATH_SRC} to ${TEST_DATA_PATH}")
        string(REGEX REPLACE "[/\\:]" "_" DATA_TARGET_NAME "${TEST_DATA_PATH_SRC}")
        if(UNIX)
            # on unix we get the third / from the filename
            set(TEST_DATA_URL "file://${TEST_DATA_PATH}")
        else()
            # we don't on windows, so we have to add it ourselves
            set(TEST_DATA_URL "file:///${TEST_DATA_PATH}")
        endif()
        if(NOT TARGET "${DATA_TARGET_NAME}")
            add_custom_target(${DATA_TARGET_NAME})
            add_dependencies(${test_name} ${DATA_TARGET_NAME})
            add_custom_command(
                TARGET ${DATA_TARGET_NAME}
                COMMAND ${CMAKE_COMMAND} "-DTEST_DATA_URL=${TEST_DATA_URL}" -DSOURCE=${TEST_DATA_PATH_SRC} -DDESTINATION=${TEST_DATA_PATH} -P ${TEST_RESOURCE_PATH}/UnitTest/generate_test_data.cmake
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            )
        endif()
    endif()

    target_link_libraries(${test_name} Qt6::Test ${OPT_LIBS})

    target_include_directories(${test_name} PRIVATE "${TEST_RESOURCE_PATH}/UnitTest/")

    add_test(NAME ${name} COMMAND ${test_name} WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

    ######################################## Workarounds #########################################
    # NOTE tell MSVC to go away and stop putting its manifests and main functions into our executable
    if (MSVC)
        set_target_properties(${test_name} PROPERTIES
            WIN32_EXECUTABLE NO
            LINK_FLAGS "/ENTRY:mainCRTStartup /MANIFEST:NO"
        )
    endif()
endfunction()
