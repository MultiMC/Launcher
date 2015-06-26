if(__SQUISHCOCOCOVERAGE_CMAKE__)
	return()
endif()
set(__SQUISHCOCOCOVERAGE_CMAKE__ TRUE)

find_program(SQUISHCOCO_CMCSEXEIMPORT_COMMAND cmcsexeimport)
find_program(SQUISHCOCO_CMMERGE_COMMAND cmmerge)
find_program(SQUISHCOCO_COVERAGEBROWSER coveragebrowser)
find_program(SQUISHCOCO_CMREPORT cmreport)

set(SQUISHCOCO_IS_COVERAGE ON CACHE BOOLEAN "" FORCE)
mark_as_advanced(SQUISHCOCO_IS_COVERAGE)

function(_append_if_not_contains var data)
	if(NOT "${${var}}" MATCHES ".*${data}.*")
		set(${var} "${${var}} ${data}" ${ARGN})
	endif()
endfunction()
function(add_coverage_flags)
	set(flags ${ARGN})
	_append_if_not_contains(CMAKE_C_FLAGS             ${flags} CACHE STRING "Flags used by the C compiler."              FORCE)
	_append_if_not_contains(CMAKE_CXX_FLAGS           ${flags} CACHE STRING "Flags used by the C++ compiler."            FORCE)
	_append_if_not_contains(CMAKE_EXE_LINKER_FLAGS    ${flags} CACHE STRING "Flags used for linking binaries."           FORCE)
	_append_if_not_contains(CMAKE_SHARED_LINKER_FLAGS ${flags} CACHE STRING "Flags used by the shared libraries linker." FORCE)
	_append_if_not_contains(CMAKE_STATIC_LINKER_FLAGS ${flags} CACHE STRING "Flags used by the static libraries linker." FORCE)
endfunction()
add_coverage_flags("--cs-on --cs-count --cs-output=%A --cs-exclude-path=${CMAKE_BINARY_DIR}")

function(setup_coverage_for)
	set(targets ${ARGN})
	set(csmes_files )
	foreach(target ${targets})
		message(STATUS "Setting up coverage for ${target}")
		target_compile_definitions(${target} PRIVATE -DSQUISHCOCO_COVERAGE -DSQUISHCOCO_CURRENTEXECUTABLE="$<TARGET_FILE:${target}>")
		add_custom_command(OUTPUT ${target}.csexe COMMAND ${target} DEPENDS ${target})
		add_custom_command(OUTPUT merged_${target}.csmes VERBATIM
			COMMENT "Collecting coverage for ${target}"
			DEPENDS ${target}.csexe
			WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
			COMMAND ${CMAKE_COMMAND} -E copy ${target}.csmes merged_${target}.csmes
			COMMAND ${SQUISHCOCO_CMCSEXEIMPORT_COMMAND} -m merged_${target}.csmes -d -t "Test execution" ${target}.csexe
		)
		list(APPEND csmes_files merged_${target}.csmes)
	endforeach()

	add_custom_command(OUTPUT ${CMAKE_PROJECT_NAME}.csmes VERBATIM
		COMMENT "Merging coverage reports"
		DEPENDS ${csmes_files}
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
		COMMAND ${SQUISHCOCO_CMMERGE_COMMAND} -o ${CMAKE_PROJECT_NAME}.csmes ${csmes_files}
	)
	add_custom_target(coverage DEPENDS ${CMAKE_PROJECT_NAME}.csmes)
	add_custom_target(coverage_browse VERBATIM
		DEPENDS coverage
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
		COMMAND ${SQUISHCOCO_COVERAGEBROWSER} ${CMAKE_PROJECT_NAME}.csmes
	)
endfunction()

include(CMakeParseArguments)
function(setup_coverage_report)
	set(options)
	set(oneValueArgs TITLE ICON CSS)
	set(multiValueArgs)
	cmake_parse_arguments(SCR "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	set(csmes_file ${CMAKE_PROJECT_NAME}.csmes)
	set(reports_base_dir "${CMAKE_BINARY_DIR}/reports/coverage")

	add_custom_target(coverage_report_html DEPENDS ${CMAKE_PROJECT_NAME}.csmes
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
		COMMAND ${SQUISHCOCO_CMREPORT} -m ${CMAKE_PROJECT_NAME}.csmes --title=${SCR_TITLE} --icon=${SCR_ICON} --css=${SCR_CSS} -h ${reports_base_dir}/html --section=global --section=function --section=class --section=execution --section=source --section=directory --section=dead-code
		SOURCES ${SCR_CSS}
	)
	add_custom_target(coverage_report_emma_xml DEPENDS ${CMAKE_PROJECT_NAME}.csmes
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
		COMMAND ${SQUISHCOCO_CMREPORT} -m ${CMAKE_PROJECT_NAME}.csmes --emma=${reports_base_dir}/emma
	)
	add_custom_target(coverage_report_cobertura DEPENDS ${CMAKE_PROJECT_NAME}.csmes
		WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
		COMMAND ${SQUISHCOCO_CMREPORT} -m ${CMAKE_PROJECT_NAME}.csmes --cobertura=${reports_base_dir}/cobertura.xml --cobertura-source=${CMAKE_SOURCE_DIR}
	)
	add_custom_target(coverage_reports DEPENDS coverage_report_html coverage_report_emma_xml coverage_report_cobertura)

	option(SQUISHCOCO_HTML_PUBLISH "Set to ON to get access to the coverage_publish_html target" OFF)
	if(SQUISHCOCO_HTML_PUBLISH)
		find_program(RSYNC_COMMAND rsync)
		find_program(SSH_COMMAND ssh)
		set(SQUISHCOCO_HTML_PUBLISH_HOST "" CACHE STRING "The host to publish Squish Coco HTML coverage report to")
		set(SQUISHCOCO_HTML_PUBLISH_USER "$ENV{USER}" CACHE STRING "The user to publish Squish Coco HTML coverage report as")
		set(SQUISHCOCO_HTML_PUBLISH_DIR "@GIT_BRANCH@" CACHE STRING "The directory on the host to publish the Squish Coco HTML coverage report to")
		string(CONFIGURE "${SQUISHCOCO_HTML_PUBLISH_DIR}" dir @ONLY)
		add_custom_target(coverage_publish_html DEPENDS coverage_report_html
			COMMAND ${SSH_COMMAND} ${SQUISHCOCO_HTML_PUBLISH_USER}@${SQUISHCOCO_HTML_PUBLISH_HOST} mkdir -p ${dir}
			COMMAND ${RSYNC_COMMAND} -az --progress ${reports_base_dir}/html_html/* ${SQUISHCOCO_HTML_PUBLISH_USER}@${SQUISHCOCO_HTML_PUBLISH_HOST}:${dir}
		)
	endif()
endfunction()
