project(classparser)

set(CMAKE_AUTOMOC ON)

# Find Qt
find_package(Qt5Core REQUIRED)

# Include Qt headers.
include_directories(${Qt5Base_INCLUDE_DIRS})

SET(CLASSPARSER_SOURCES
 include/classparser_config.h

 include/javautils.h
 src/javautils.cpp

 src/ClassFile.h
 src/ClassFile.cpp
 src/ConstantPool.h
 src/ConstantPool.cpp
 src/Attribute.h
 src/Attribute.cpp
)

# Set the include dir path.
SET(LIBGROUPVIEW_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include" PARENT_SCOPE)

# Include self.
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_BINARY_DIR}/include)

add_definitions(-DCLASSPARSER_STATIC)

add_library(classparser STATIC ${CLASSPARSER_SOURCES})
target_link_libraries(classparser quazip)
qt5_use_modules(classparser Core)
