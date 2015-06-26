find_program(SQUISHCOCO_COVERAGE_GCC csgcc)
find_program(SQUISHCOCO_COVERAGE_GXX csg++)
find_program(SQUISHCOCO_COVERAGE_AR csar)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	message(WARNING "Your build type is Debug. You should use Release for coverage")
endif()
include(${CMAKE_CURRENT_LIST_DIR}/SquishCocoCoverage.cmake)

set(CMAKE_C_COMPILER "${SQUISHCOCO_COVERAGE_GCC}"
    CACHE FILEPATH "CoverageScanner wrapper" FORCE)
set(CMAKE_CXX_COMPILER "${SQUISHCOCO_COVERAGE_GXX}"
    CACHE FILEPATH "CoverageScanner wrapper" FORCE)
set(CMAKE_LINKER "${SQUISHCOCO_COVERAGE_GXX}"
    CACHE FILEPATH "CoverageScanner wrapper" FORCE)
set(CMAKE_AR "${SQUISHCOCO_COVERAGE_AR}"
    CACHE FILEPATH "CoverageScanner wrapper" FORCE)
