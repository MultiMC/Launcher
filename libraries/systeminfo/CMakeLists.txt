project(systeminfo)

find_package(Qt5Core)

set(systeminfo_SOURCES
    include/sys.h
    include/distroutils.h
    src/distroutils.cpp
)

set(systeminfo_INCLUDE_DIRS
        ${CMAKE_CURRENT_LIST_DIR}/include)

if (WIN32)
    set(systeminfo_BUILD_SOURCES
            build-src/win32/main.cpp)

    set(systeminfo_PREPROCESSOR_INPUT
            build-src/win32/ntstatus.preprocess.h)

    set(systeminfo_PREPROCESSOR_OUT
            ${CMAKE_CURRENT_BINARY_DIR}/ntstatus.preprocess.h.target)

    get_filename_component(systeminfo_PREPROCESSOR_INPUT_FULL
            ${systeminfo_PREPROCESSOR_INPUT}
            REALPATH BASE_DIR ${CMAKE_CURRENT_LIST_DIR}
            )

    add_custom_target(systeminfo-preprocess-nstatus)
    add_custom_command(
            COMMAND ${CMAKE_C_COMPILER} -MF ${systeminfo_PREPROCESSOR_OUT} -M ${systeminfo_PREPROCESSOR_INPUT_FULL}
            DEPENDS ${systeminfo_PREPROCESSOR_INPUT}
            COMMENT "Generating path to ntstatus.h"
            TARGET "systeminfo-preprocess-nstatus"
    )

    add_executable(systeminfo-ntstatus-gen ${systeminfo_BUILD_SOURCES})
    add_dependencies(systeminfo-ntstatus-gen systeminfo-preprocess-nstatus)
    target_compile_definitions(systeminfo-ntstatus-gen PRIVATE
            NTSTATUS_PREPROCESSOR_OUT=\"${systeminfo_PREPROCESSOR_OUT}\")

    set(NTSTATUS_GEN_HEADER "${CMAKE_CURRENT_BINARY_DIR}/NtStatsuGen.h")
    set(NTSTATUS_GEN_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/NtStatusGen.cpp")

    add_custom_command(
            COMMAND systeminfo-ntstatus-gen ${NTSTATUS_GEN_HEADER} ${NTSTATUS_GEN_SOURCE}
            COMMENT "Generating NTSTATUS lookup table"
            OUTPUT ${NTSTATUS_GEN_HEADER} ${NTSTATUS_GEN_SOURCE}
    )
    set_property(SOURCE ${NTSTATUS_GEN_HEADER} PROPERTY SKIP_AUTOMOC ON)
    set_property(SOURCE ${NTSTATUS_GEN_SOURCE} PROPERTY SKIP_AUTOMOC ON)

    list(APPEND systeminfo_SOURCES src/sys_win32.cpp ${NTSTATUS_GEN_SOURCE} ${NTSTATUS_GEN_HEADER})
    list(APPEND systeminfo_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR})
elseif (UNIX)
    if(APPLE)
        list(APPEND systeminfo_SOURCES src/sys_apple.cpp)
    else()
        list(APPEND systeminfo_SOURCES src/sys_unix.cpp)
    endif()
endif()

add_library(systeminfo STATIC ${systeminfo_SOURCES})
target_link_libraries(systeminfo Qt5::Core Qt5::Gui Qt5::Network)
target_include_directories(systeminfo PUBLIC ${systeminfo_INCLUDE_DIRS})

include (UnitTest)
add_unit_test(sys
    SOURCES src/sys_test.cpp
    LIBS systeminfo
)
