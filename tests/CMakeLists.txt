set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5 COMPONENTS Test Core Network Widgets)

include_directories(..)

add_definitions(-DMULTIMC_TEST)

unset(MultiMC_TESTS)
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

macro(add_unit_test2 name)
    add_unit_test(${name} tst_${name}.cpp)
endmacro()

add_unit_test2(QuickMod)
add_unit_test2(QuickModsList)

set(COVERAGE_SOURCE_DIRS
 ${SOURCE_ROOT}/logic/*
 ${SOURCE_ROOT}/logic/lists/*
 ${SOURCE_ROOT}/logic/net/*
 ${SOURCE_ROOT}/logic/tasks/*
 ${SOURCE_ROOT}/gui/*
 ${SOURCE_ROOT}/gui/dialogs/*
 ${SOURCE_ROOT}/gui/widgets/*
 ${SOURCE_ROOT}/depends/classparser/include/*
 ${SOURCE_ROOT}/depends/classparser/src/*
 ${SOURCE_ROOT}/depends/launcher/include/*
 ${SOURCE_ROOT}/depends/launcher/src/*
 ${SOURCE_ROOT}/depends/classparser/include/*
 ${SOURCE_ROOT}/depends/classparser/src/*
 ${SOURCE_ROOT}/depends/settings/include/*
 ${SOURCE_ROOT}/depends/settings/src/*
 ${SOURCE_ROOT}/depends/utils/include/*
 ${SOURCE_ROOT}/depends/utils/src/*
)

if(MultiMC_CODE_COVERAGE)
    unset(MultiMC_RUN_TESTS)
    unset(MultiMC_TEST_COVERAGE_FILES)

    foreach(test ${MultiMC_TESTS})
	add_custom_target(MultiMC_RUN_TEST_${test}
	 COMMAND lcov -d ${CMAKE_CURRENT_BINARY_DIR} -z -q # clean test
		 && lcov -d ${BUILD_ROOT} -z -q # clean common
		 && ${BUILD_ROOT}/${test} -o coverage_${test}.out,xml # run test
		 && lcov --checksum -b ${SOURCE_ROOT} -d ${CMAKE_CURRENT_BINARY_DIR} -c -o coverage_${test}.info # generate for test
		 && lcov --checksum -b ${SOURCE_ROOT} -d ${BUILD_ROOT} -c -o coverage_common.info # generate for common
		 && lcov --checksum -b ${SOURCE_ROOT} -d . -a coverage_${test}.info -a coverage_common.info -o coverage_${test}-combined.info # combine test and common
		 && lcov --checksum -b ${SOURCE_ROOT} --list-full-path --extract coverage_${test}-combined.info ${COVERAGE_SOURCE_DIRS} -o coverage_${test}-stripped.info # strip
	 WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	 VERBATIM
	 DEPENDS ${test}
	 COMMENT "Running ${test}..."
	)
	list(APPEND MultiMC_TEST_COVERAGE_FILES coverage_${test}-stripped.info)
	list(APPEND MultiMC_RUN_TESTS MultiMC_RUN_TEST_${test})
    endforeach(test)

    add_custom_target(MultiMC_GENERATE_COVERAGE
     DEPENDS ${MultiMC_RUN_TESTS}
     COMMENT "Generating coverage files..."
    )
    add_custom_target(MultiMC_GENERATE_COVERAGE_HTML
     COMMAND genhtml -t "MultiMC 5 Test Coverage" --num-spaces 4 --demangle-cpp --legend -o ${SOURCE_ROOT}/html/coverage ${MultiMC_TEST_COVERAGE_FILES}
     DEPENDS MultiMC_GENERATE_COVERAGE
     COMMENT "Generating test coverage html..."
    )
    add_custom_target(MultiMC_RUN_TESTS DEPENDS MultiMC_GENERATE_COVERAGE_HTML)
endif(MultiMC_CODE_COVERAGE)

add_subdirectory(data)
