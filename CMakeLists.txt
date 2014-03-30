cmake_minimum_required(VERSION 2.8.9)

if(WIN32)
      # In Qt 5.1+ we have our own main() function, don't autolink to qtmain on Windows
      cmake_policy(SET CMP0020 OLD)
endif()

project(MultiMC)
enable_testing()

include(FeatureSummary)

######## Set CMake options ########
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(FILES_TO_TRANSLATE )

######## Set module path ########
set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake/" "${PROJECT_SOURCE_DIR}")
set(MMC_SRC "${PROJECT_SOURCE_DIR}")
set(MMC_BIN "${PROJECT_BINARY_DIR}")

# Output all executables and shared libs in the main build folder, not in subfolders.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

if(UNIX)
      set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
endif()

set(CMAKE_JAVA_TARGET_OUTPUT_DIR ${PROJECT_BINARY_DIR}/jars)

######## Set compiler flags ########
include(UseCXX11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Werror=return-type")

################################ 3rd Party Libs ################################

if(UNIX AND NOT APPLE)
    set(MultiMC_QT_ADDITIONAL_MODULES ${MultiMC_QT_ADDITIONAL_MODULES} X11Extras)
    set(MultiMC_LINK_ADDITIONAL_LIBS ${MultiMC_LINK_ADDITIONAL_LIBS} xcb)
elseif(WIN32)
    set(MultiMC_LINK_ADDITIONAL_LIBS ${MultiMC_LINK_ADDITIONAL_LIBS}
     Qt5::WinMain # Link WinMain
    )
endif()

find_package(Cotire)
set_package_properties(Cotire PROPERTIES URL "https://github.com/sakra/cotire" PURPOSE "Speed up builds" TYPE RECOMMENDED)

find_package(Git QUIET)

# Find the required Qt parts
find_package(Qt5 QUIET REQUIRED COMPONENTS Core Widgets Network Concurrent Xml ${MultiMC_QT_ADDITIONAL_MODULES})
find_package(Qt5Test)
find_package(Qt5LinguistTools)
set_package_properties(Qt5 PROPERTIES URL "https://qt-project.org/" TYPE REQUIRED)
set_package_properties(Qt5Test PROPERTIES URL "https://qt-project.org/" DESCRIPTION "Qt unit testing framework" TYPE OPTIONAL)
set_package_properties(Qt5LinguistTools PROPERTIES URL "https://qt-project.org/" DESCRIPTION "Translation tools for Qt" TYPE OPTIONAL)

include_directories(
      ${Qt5Core_INCLUDE_DIRS}
      ${Qt5Widgets_INCLUDE_DIRS}
      ${Qt5Concurrent_INCLUDE_DIRS}
      ${Qt5Network_INCLUDE_DIRS}
      ${Qt5Test_INCLUDE_DIRS}
  )

# The Qt5 cmake files don't provide its install paths, so ask qmake.
include(QMakeQuery)
query_qmake(QT_INSTALL_PLUGINS QT_PLUGINS_DIR)
query_qmake(QT_INSTALL_IMPORTS QT_IMPORTS_DIR)
query_qmake(QT_INSTALL_LIBS QT_LIBS_DIR)
query_qmake(QT_HOST_DATA QT_DATA_DIR)
set(QT_MKSPECS_DIR ${QT_DATA_DIR}/mkspecs)


################################ SET UP BUILD OPTIONS ################################

######## Set URLs ########

set(MultiMC_NEWS_RSS_URL "http://multimc.org/rss.xml" CACHE STRING "URL to fetch MultiMC's news RSS feed from.")
set(MultiMC_CHANLIST_URL "" CACHE STRING "URL for the channel list.")
set(MultiMC_NOTIFICATION_URL "" CACHE STRING "URL for checking for notifications.")

######## Set version numbers ########
set(MultiMC_VERSION_MAJOR		0)
set(MultiMC_VERSION_MINOR		3)
set(MultiMC_VERSION_HOTFIX		0)

set(MultiMC_VERSION_BUILD -1 CACHE STRING "Build number. -1 for no build number.")
set(MultiMC_VERSION_TYPE "Custom" CACHE STRING "MultiMC's version type. This should be one of 'Custom', 'Release', 'ReleaseCandidate', or 'Development', depending on what type of version this is.")
set(MultiMC_BUILD_PLATFORM "" CACHE STRING "A short string identifying the platform that this build was built for. Only used by the notification system and to display in the about dialog.")
set(MultiMC_VERSION_CHANNEL "" CACHE STRING "The current build's channel. Included in the version string.")

# Updater enabled?
option(MultiMC_UPDATER "Whether or not the update system is enabled. If this is enabled, you must also set MultiMC_CHANLIST_URL and MultiMC_VERSION_CHANNEL in order for it to work properly." FALSE)
add_feature_info(Updater MultiMC_UPDATER "Build and package the updater")

set(MultiMC_RELEASE_VERSION_NAME "${MultiMC_VERSION_MAJOR}.${MultiMC_VERSION_MINOR}")
if(MultiMC_VERSION_HOTFIX GREATER 0)
    set(MultiMC_RELEASE_VERSION_NAME "${MultiMC_RELEASE_VERSION_NAME}.${MultiMC_VERSION_HOTFIX}")
endif()

# Build a version string to display in the configure logs.
if(MultiMC_VERSION_TYPE STREQUAL "Custom")
    message(STATUS "Version Type: Custom")
    set(MultiMC_VERSION_STRING "${MultiMC_RELEASE_VERSION_NAME}")
elseif(MultiMC_VERSION_TYPE STREQUAL "Release")
    message(STATUS "Version Type: Stable Release")
    set(MultiMC_VERSION_STRING "${MultiMC_RELEASE_VERSION_NAME}")
elseif(MultiMC_VERSION_TYPE STREQUAL "ReleaseCandidate")
    message(STATUS "Version Type: Release Candidate")
    set(MultiMC_VERSION_STRING "${MultiMC_RELEASE_VERSION_NAME}-rc${MultiMC_VERSION_BUILD}")
elseif(MultiMC_VERSION_TYPE STREQUAL "Development")
    message(STATUS "Version Type: Development")
    set(MultiMC_VERSION_STRING "${MultiMC_RELEASE_VERSION_NAME}-dev${MultiMC_VERSION_BUILD}")
else()
    message(ERROR "Invalid build type.")
endif()

message(STATUS "MultiMC 5 Version: ${MultiMC_VERSION_STRING}")

# If the update system is enabled, make sure MultiMC_CHANLIST_URL and MultiMC_VERSION_CHANNEL are set.
if(MultiMC_UPDATER)
    if(MultiMC_VERSION_CHANNEL STREQUAL "")
	message(FATAL_ERROR "Update system is enabled, but MultiMC_VERSION_CHANNEL is not set.\n"
			    "Please ensure the CMake variables MultiMC_VERSION_CHANNEL, MultiMC_CHANLIST_URL, and MultiMC_VERSION_BUILD are set.")
    endif()
    if(MultiMC_CHANLIST_URL STREQUAL "")
	message(FATAL_ERROR "Update system is enabled, but MultiMC_CHANLIST_URL is not set.\n"
			    "Please ensure the CMake variables MultiMC_VERSION_CHANNEL, MultiMC_CHANLIST_URL, and MultiMC_VERSION_BUILD are set.")
    endif()
    if(MultiMC_VERSION_BUILD LESS 0)
	message(FATAL_ERROR "Update system is enabled, but MultiMC_VERSION_BUILD is not set.\n"
			    "Please ensure the CMake variables MultiMC_VERSION_CHANNEL, MultiMC_CHANLIST_URL, and MultiMC_VERSION_BUILD are set.")
    endif()

    message(STATUS "Updater is enabled. Channel list URL: ${MultiMC_CHANLIST_URL}")
endif()

#### Custom target to just print the version.
add_custom_target(version echo "Version: ${MultiMC_VERSION_STRING}")

#### Check the current Git commit
if(GIT_FOUND)
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
     WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
     RESULT_VARIABLE GIT_COMMIT_CHECK_RESULTVAR
     OUTPUT_VARIABLE GIT_COMMIT_CHECK_OUTVAR
     OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(GIT_COMMIT_CHECK_RESULTVAR EQUAL 0)
	set(MultiMC_GIT_COMMIT "${GIT_COMMIT_CHECK_OUTVAR}")
	message(STATUS "Git commit: ${MultiMC_GIT_COMMIT}")
    else()
	set(MultiMC_GIT_COMMIT "Unknown")
	message(STATUS "Failed to check Git commit. ${GIT_COMMIT_CHECK_RESULTVAR}")
    endif()
else()
    set(MultiMC_GIT_COMMIT "Unknown")
endif()

######## Configure header ########
configure_file("${PROJECT_SOURCE_DIR}/config.h.in" "${PROJECT_BINARY_DIR}/include/config.h")
include_directories(${PROJECT_BINARY_DIR}/include)

######## Packaging/install paths setup ########

if(UNIX AND APPLE)
      set(BINARY_DEST_DIR MultiMC.app/Contents/MacOS)
      set(PLUGIN_DEST_DIR MultiMC.app/Contents/MacOS)
      set(QTCONF_DEST_DIR MultiMC.app/Contents/Resources)
      set(APPS "\${CMAKE_INSTALL_PREFIX}/MultiMC.app")

      set(MACOSX_BUNDLE_BUNDLE_NAME "MultiMC")
      set(MACOSX_BUNDLE_INFO_STRING "MultiMC Minecraft launcher and management utility.")
      set(MACOSX_BUNDLE_GUI_IDENTIFIER "org.multimc.MultiMC5")
      set(MACOSX_BUNDLE_BUNDLE_VERSION "${MultiMC_VERSION_MAJOR}.${MultiMC_VERSION_MINOR}.${MultiMC_VERSION_REV}.${MultiMC_VERSION_BUILD}")
      set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${MultiMC_VERSION_MAJOR}.${MultiMC_VERSION_MINOR}.${MultiMC_VERSION_REV}.${MultiMC_VERSION_BUILD}")
      set(MACOSX_BUNDLE_LONG_VERSION_STRING "${MultiMC_VERSION_MAJOR}.${MultiMC_VERSION_MINOR}.${MultiMC_VERSION_REV}.${MultiMC_VERSION_BUILD}")
      set(MACOSX_BUNDLE_ICON_FILE MultiMC.icns)
      set(MACOSX_BUNDLE_COPYRIGHT "Copyright 2013 MultiMC Contributors")
elseif(UNIX)
      set(BINARY_DEST_DIR bin)
      set(PLUGIN_DEST_DIR plugins)
      set(QTCONF_DEST_DIR .)
      set(APPS "\${CMAKE_INSTALL_PREFIX}/bin/MultiMC")
elseif(WIN32)
      set(BINARY_DEST_DIR .)
      set(PLUGIN_DEST_DIR .)
      set(QTCONF_DEST_DIR .)
      set(APPS "\${CMAKE_INSTALL_PREFIX}/MultiMC.exe")
endif()

# directories to look for dependencies
set(DIRS "${QT_LIBS_DIR}")

################################ Included Libs ################################

# Add quazip
add_subdirectory(depends/quazip)
include_directories(depends/quazip)

# Add the java launcher and checker
add_subdirectory(depends/launcher)
add_subdirectory(depends/javacheck)

# Add xz decompression
add_subdirectory(depends/xz-embedded)
include_directories(${XZ_INCLUDE_DIR})

# Add pack200 decompression
add_subdirectory(depends/pack200)
include_directories(${PACK200_INCLUDE_DIR})

######## MultiMC Libs ########

# Add the util library.
add_subdirectory(depends/util)
include_directories(${LIBUTIL_INCLUDE_DIR})

# Add the settings library.
add_subdirectory(depends/settings)
include_directories(${LIBSETTINGS_INCLUDE_DIR})

# Add the updater
if(MultiMC_UPDATER)
    add_subdirectory(mmc_updater)
endif()

################################ FILES ################################

add_subdirectory(logic)
add_subdirectory(gui)
add_subdirectory(logger)

set(MULTIMC_SOURCES
 # Application base
 MultiMC.h
 MultiMC.cpp
 MultiMCVersion.h
 MMCError.h

 ${MULTIMC_LOGGER_SRCS}
 ${MULTIMC_LOGIC_SRCS}
 ${MULTIMC_GUI_SRCS}
)

set(MULTIMC_UIS
 ${MULTIMC_GUI_UIS}
)

set(FILES_TO_TRANSLATE ${FILES_TO_TRANSLATE} ${MULTIMC_SOURCES} ${MULTIMC_UIS})

set(MULTIMC_QRCS
resources/backgrounds/backgrounds.qrc
resources/multimc/multimc.qrc
resources/instances/instances.qrc
)

######## Windows resource files ########
if(WIN32)
    set(MULTIMC_RCS resources/multimc.rc)
endif()

################################ COMPILE ################################

option(MultiMC_UPDATER_DRY_RUN "Enable updater dry-run mode -- for updater development." OFF)
option(MultiMC_UPDATER_FORCE_LOCAL "Do not download updated updater -- for updater development." OFF)

option(MultiMC_CODE_COVERAGE "Compiles for code coverage" OFF)
if(MultiMC_CODE_COVERAGE)
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 --coverage")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 --coverage")
      set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 --coverage")
      set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g -O0 --coverage")
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -O0 --coverage")
endif(MultiMC_CODE_COVERAGE)
add_feature_info(Coverage MultiMC_CODE_COVERAGE "Compiles for code coverage")

# Qt 5 stuff
qt5_wrap_ui(MULTIMC_UI ${MULTIMC_UIS})
qt5_add_resources(MULTIMC_RESOURCES ${MULTIMC_QRCS})

# Add common library
add_library(MultiMC_common STATIC ${MULTIMC_SOURCES} ${MULTIMC_UI} ${MULTIMC_RESOURCES})

# Add executable
add_executable(MultiMC MACOSX_BUNDLE WIN32 main.cpp ${MULTIMC_RCS})

# Link
target_link_libraries(MultiMC MultiMC_common)
target_link_libraries(MultiMC_common xz-embedded unpack200 quazip libUtil libSettings ${MultiMC_LINK_ADDITIONAL_LIBS})
qt5_use_modules(MultiMC Core Widgets Network Xml Concurrent ${MultiMC_QT_ADDITIONAL_MODULES})
qt5_use_modules(MultiMC_common Core Widgets Network Xml Concurrent ${MultiMC_QT_ADDITIONAL_MODULES})

cotire(MultiMC_common)

################################ INSTALLATION AND PACKAGING ################################

######## Install ########

#### Executable ####
if(APPLE AND UNIX) # OSX
    install(TARGETS MultiMC
     BUNDLE DESTINATION . COMPONENT Runtime
     RUNTIME DESTINATION MultiMC.app/Contents/MacOS COMPONENT Runtime
    )
elseif(UNIX) # Linux and similar
    install(TARGETS MultiMC
     BUNDLE DESTINATION . COMPONENT Runtime
     RUNTIME DESTINATION bin COMPONENT Runtime
    )
    install(PROGRAMS package/linux/MultiMC DESTINATION .)
elseif(WIN32) # Windows
    install(TARGETS MultiMC
     BUNDLE DESTINATION . COMPONENT Runtime
     LIBRARY DESTINATION . COMPONENT Runtime
     RUNTIME DESTINATION . COMPONENT Runtime
    )
endif()

#### Dist package logic ####

if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    # Image formats
    install(
     DIRECTORY "${QT_PLUGINS_DIR}/imageformats"
     DESTINATION ${PLUGIN_DEST_DIR}
     COMPONENT Runtime
     REGEX "tga|svg|tiff|mng" EXCLUDE
    )

    # Platform plugins
    install(
     DIRECTORY "${QT_PLUGINS_DIR}/platforms"
     DESTINATION ${PLUGIN_DEST_DIR}
     COMPONENT Runtime
     REGEX "minimal|linuxfb|offscreen" EXCLUDE
    )
else()
    # Image formats
    install(
     DIRECTORY "${QT_PLUGINS_DIR}/imageformats"
     DESTINATION ${PLUGIN_DEST_DIR}
     COMPONENT Runtime
     REGEX "tga|svg|tiff|mng" EXCLUDE
     REGEX "d\\." EXCLUDE
     REGEX "_debug\\." EXCLUDE
    )

    # Platform plugins
    install(
     DIRECTORY "${QT_PLUGINS_DIR}/platforms"
     DESTINATION ${PLUGIN_DEST_DIR}
     COMPONENT Runtime
     REGEX "minimal|linuxfb|offscreen" EXCLUDE
     REGEX "d\\." EXCLUDE
     REGEX "_debug\\." EXCLUDE
    )
    if(APPLE)
	# Accessible plugin to make buttons look decent on osx
	install(
	 DIRECTORY "${QT_PLUGINS_DIR}/accessible"
	 DESTINATION ${PLUGIN_DEST_DIR}
	 COMPONENT Runtime
	 REGEX "quick" EXCLUDE
	 REGEX "d\\." EXCLUDE
	 REGEX "_debug\\." EXCLUDE
	)
    endif()
endif()

# qtconf
install(
 CODE "
FILE(WRITE \"\${CMAKE_INSTALL_PREFIX}/${QTCONF_DEST_DIR}/qt.conf\" \"\")
"
 COMPONENT Runtime
)

# ICNS file for OS X
if(APPLE)
    install(FILES resources/MultiMC.icns DESTINATION MultiMC.app/Contents/Resources)
endif()

configure_file(
      "${CMAKE_CURRENT_SOURCE_DIR}/install_prereqs.cmake.in"
      "${CMAKE_CURRENT_BINARY_DIR}/install_prereqs.cmake"
      @ONLY)
install(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/install_prereqs.cmake" COMPONENT Runtime)

# Package with CPack
include(CPackConfig)
include(CPack)

# Translations
add_feature_info(Translations Qt5LinguistTools_FOUND "Translations")
if(Qt5LinguistTools_FOUND)
    add_subdirectory(translations)
endif()

# Tests
add_feature_info(Tests Qt5Test_FOUND "Unit tests")
if(Qt5Test_FOUND)
    add_subdirectory(tests)
endif()

if(NOT __FIRST_RUN__)
    feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
    set(__FIRST_RUN__ TRUE CACHE BOOLEAN "")
    mark_as_advanced(__FIRST_RUN__)
endif()
