project(classparser)

set(CMAKE_AUTOMOC ON)

######## Check endianness ########
include(TestBigEndian)
test_big_endian(BIGENDIAN)
if(${BIGENDIAN})
    add_definitions(-DMULTIMC_BIG_ENDIAN)
endif(${BIGENDIAN})

# Find Qt
find_package(Qt6 REQUIRED COMPONENTS Core)

set(CLASSPARSER_HEADERS
# Public headers
include/classparser_config.h
include/classparser.h

# Private headers
src/annotations.h
src/classfile.h
src/constants.h
src/errors.h
src/javaendian.h
src/membuffer.h
)

set(CLASSPARSER_SOURCES
src/classparser.cpp
src/annotations.cpp
)

add_definitions(-DCLASSPARSER_LIBRARY)

add_library(Launcher_classparser STATIC ${CLASSPARSER_SOURCES} ${CLASSPARSER_HEADERS})
target_include_directories(Launcher_classparser PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(Launcher_classparser Launcher_quazip Qt6::Core)
