# run the unit tests with `make test`
find_package(Qt5 REQUIRED COMPONENTS Test Core Network)

include_directories(${MMC_SRC} ${CMAKE_CURRENT_SOURCE_DIR} ${MMC_SRC}/logic ${MMC_SRC}/depends/util/include)

# Tests START #

# a test is a file named tst_<name>.cpp anywhere in a subdirectory of the current directory
# it can contain an include for ModelTester.h and/or AccountTester.h, in which case they will be
# compiled into the executable
file(GLOB_RECURSE tests "tst_*.cpp")
set(test_targets )
foreach(test ${tests})
	# Extract the name from the source file
	get_filename_component(name "${test}" NAME_WE)
	string(REPLACE "tst_" "" name "${name}")

	# Gather a list of source files
	set(src ${test} TestUtil.h)
	file(READ "${test}" test_contents)
	if("${test_contents}" MATCHES "ModelTester\\.h")
		list(APPEND src ModelTester.h)
	endif()

	# Extra file needed on windows
	if(WIN32)
		list(APPEND src ${CMAKE_CURRENT_SOURCE_DIR}/test.rc)
	endif()

	# Create and set up the executable
	add_executable(tst_${name} ${src})
	target_link_libraries(tst_${name} Qt5::Core Qt5::Test Qt5::Network MultiMC_logic)
	add_test(NAME ${name} COMMAND tst_${name})
	list(APPEND test_targets tst_${name})
endforeach()

# Tests END #

if(SQUISHCOCO_IS_COVERAGE)
	set(GIT_BRANCH ${MultiMC_GIT_REF})
	setup_coverage_for(${test_targets})
	# Exclude things in depends/ (keep utils tho), qrc_*.cpp files, tests and
	add_coverage_flags("--cs-exclude-file-regex='.*/depends/[^u].*' --cs-exclude-file-regex=qrc_.* --cs-exclude-path=${CMAKE_CURRENT_SOURCE_DIR}")
	setup_coverage_report(TITLE "MultiMC Coverage" ICON "${MMC_SRC}/application/resources/MultiMC.ico" CSS "${CMAKE_CURRENT_SOURCE_DIR}/codecoverage.css")
endif(SQUISHCOCO_IS_COVERAGE)

set(MultiMC_TEST_DATA_PATH "${CMAKE_CURRENT_BINARY_DIR}/data")
if(UNIX)
	# on unix we get the third / from the filename
	set(MultiMC_TEST_DATA_PATH "file://${MultiMC_TEST_DATA_PATH}")
else()
	# we don't on windows, so we have to add it ourselves
	set(MultiMC_TEST_DATA_PATH "file:///${MultiMC_TEST_DATA_PATH}")
endif()
file(GLOB data_files "data/*")
foreach(data_file ${data_files})
	if (NOT IS_DIRECTORY ${data_file})
		get_filename_component(filename ${data_file} NAME)
		configure_file(
			${data_file}
			${CMAKE_CURRENT_BINARY_DIR}/data/${filename}
			@ONLY
			NEWLINE_STYLE LF
		)
	endif()
endforeach()
file(GLOB binary_files "data/binary/*")
foreach(binary_file ${binary_files})
	get_filename_component(filename ${binary_file} NAME)
	configure_file(${binary_file} ${CMAKE_CURRENT_BINARY_DIR}/data/${filename} COPYONLY)
endforeach()

configure_file(test_config.h.in test_config.h @ONLY)
