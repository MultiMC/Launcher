set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5 COMPONENTS Test Core Network Widgets)

include_directories(..)
#include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_definitions(-DMULTIMC_TEST)

macro(add_unit_test name)
    unset(srcs)
    foreach(arg ${testname} ${ARGN})
	list(APPEND srcs ${CMAKE_CURRENT_SOURCE_DIR}/${arg})
    endforeach()
    add_executable(tst_${name} ${srcs})
    qt5_use_modules(tst_${name} Test Core Network Widgets)
    target_link_libraries(tst_${name} MultiMC_common)
    add_test(tst_${name} tst_${name})
    list(APPEND MultiMC_TESTS tst_${name})
endmacro()

add_unit_test(QuickMod tst_QuickMod.cpp)

add_subdirectory(data)
