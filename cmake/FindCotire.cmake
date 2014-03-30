if(__FINDCOTIRE_CMAKE__)
    return()
endif()
set(__FINDCOTIRE_CMAKE__ TRUE)

if(NOT EXISTS "cmake/cotire.cmake" OR COTIRE_FORCE_DOWNLOAD)
    message(STATUS "Updating cotire.cmake...")
    file(DOWNLOAD "https://github.com/sakra/cotire/raw/master/CMake/cotire.cmake" "cmake/cotire.cmake" SHOW_PROGRESS)
endif()

include(cotire OPTIONAL RESULT_VARIABLE COTIRE_FOUND_local)

if(COTIRE_FOUND_local)
    set(COTIRE_FOUND TRUE)
    mark_as_advanced(
     COTIRE_ADDITIONAL_PREFIX_HEADER_IGNORE_EXTENSIONS
     COTIRE_ADDITIONAL_PREFIX_HEADER_IGNORE_PATH
     COTIRE_DEBUG COTIRE_MAXIMUM_NUMBER_OF_UNITY_INCLUDES
     COTIRE_MINIMUM_NUMBER_OF_TARGET_SOURCES
     COTIRE_UNITY_SOURCE_EXCLUDE_EXTENSIONS
     COTIRE_VERBOSE
    )
else()
    function(cotire)
    endfunction()
endif()

mark_as_advanced(__FINDCOTIRE_CMAKE__)
