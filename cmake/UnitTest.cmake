find_package(Qt5Test REQUIRED)

set(TEST_RESOURCE_PATH ${CMAKE_CURRENT_LIST_DIR})

message(${TEST_RESOURCE_PATH})

function(add_unit_test name)
    set(options "")
    set(oneValueArgs DATA)
    set(multiValueArgs SOURCES LIBS)

    cmake_parse_arguments(OPT "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

    if(WIN32)
        add_executable(${name}_test ${OPT_SOURCES} ${TEST_RESOURCE_PATH}/UnitTest/test.rc)
    else()
        add_executable(${name}_test ${OPT_SOURCES})
    endif()

    if(NOT "${OPT_DATA}" STREQUAL "")
        set(TEST_DATA_PATH "${CMAKE_CURRENT_BINARY_DIR}/data")
        set(TEST_DATA_PATH_SRC "${CMAKE_CURRENT_SOURCE_DIR}/${OPT_DATA}")
        message("From ${TEST_DATA_PATH_SRC} to ${TEST_DATA_PATH}")
        string(MAKE_C_IDENTIFIER "${TEST_DATA_PATH_SRC}" DATA_TARGET_NAME)
        if(UNIX)
            # on unix we get the third / from the filename
            set(TEST_DATA_URL "file://${TEST_DATA_PATH}")
        else()
            # we don't on windows, so we have to add it ourselves
            set(TEST_DATA_URL "file:///${TEST_DATA_PATH}")
        endif()
        # Match QUrl::FullyEncoded in fixtures, including source/build paths with spaces.
        string(REPLACE "%" "%25" TEST_DATA_URL "${TEST_DATA_URL}")
        string(REPLACE " " "%20" TEST_DATA_URL "${TEST_DATA_URL}")
        string(REPLACE "#" "%23" TEST_DATA_URL "${TEST_DATA_URL}")
        string(REPLACE "?" "%3F" TEST_DATA_URL "${TEST_DATA_URL}")
        if(NOT TARGET "${DATA_TARGET_NAME}")
            add_custom_target(${DATA_TARGET_NAME})
            add_dependencies(${name}_test ${DATA_TARGET_NAME})
            add_custom_command(
                POST_BUILD
                TARGET ${DATA_TARGET_NAME}
                COMMAND ${CMAKE_COMMAND} "-DTEST_DATA_URL=${TEST_DATA_URL}" -DSOURCE=${TEST_DATA_PATH_SRC} -DDESTINATION=${TEST_DATA_PATH} -P ${TEST_RESOURCE_PATH}/UnitTest/generate_test_data.cmake
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            )
        endif()
    endif()

    target_link_libraries(${name}_test Qt5::Test ${OPT_LIBS})

    target_include_directories(${name}_test PRIVATE "${TEST_RESOURCE_PATH}/UnitTest/")

    add_test(NAME ${name} COMMAND ${name}_test WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endfunction()
