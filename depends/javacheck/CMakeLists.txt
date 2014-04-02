cmake_minimum_required(VERSION 2.8.6)
project(launcher Java)
find_package(Java 1.6 QUIET REQUIRED COMPONENTS Development)

include(UseJava)
set(CMAKE_JAVA_JAR_ENTRY_POINT JavaCheck)
set(CMAKE_JAVA_COMPILE_FLAGS -target 1.6 -source 1.6 -Xlint:deprecation -Xlint:unchecked)
mark_as_advanced(CMAKE_Java_ARCHIVE CMAKE_Java_RUNTIME)

add_jar(JavaCheck JavaCheck.java)

install_jar(JavaCheck "${BINARY_DEST_DIR}/jars")
