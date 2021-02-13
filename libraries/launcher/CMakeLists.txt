cmake_minimum_required(VERSION 3.1)
project(launcher Java)
find_package(Java 1.6 REQUIRED COMPONENTS Development)

include(UseJava)
set(CMAKE_JAVA_JAR_ENTRY_POINT org.launcher.EntryPoint)
set(CMAKE_JAVA_COMPILE_FLAGS -target 1.6 -source 1.6 -Xlint:deprecation -Xlint:unchecked)

set(SRC
    org/launcher/EntryPoint.java
    org/launcher/Launcher.java
    org/launcher/LegacyFrame.java
    org/launcher/NotFoundException.java
    org/launcher/ParamBucket.java
    org/launcher/ParseException.java
    org/launcher/Utils.java
    org/launcher/onesix/OneSixLauncher.java
    net/minecraft/Launcher.java
)
add_jar(NewLaunch ${SRC})
install_jar(NewLaunch "${JARS_DEST_DIR}")
