cmake_minimum_required(VERSION 3.21)

## Main build script. It all begins here.

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

##
# NOTE: we need to set these first
# Normally, these would be passed in on command line during the configuration step, but it's too annoying to rememeber to do that.
#
# See: https://cmake.org/cmake/help/latest/variable/CMAKE_OSX_ARCHITECTURES.html
# See: https://cmake.org/cmake/help/latest/variable/CMAKE_OSX_DEPLOYMENT_TARGET.html
if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "macOS architecture(s) to build for.")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "minimum macOS versions to build for.")
endif()

set(CMAKE_CXX_STANDARD_REQUIRED true)
set(CMAKE_C_STANDARD_REQUIRED true)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

# Include some common functionality for building Qt apps (from `cmake` folder)
include(QtCommon)

set(BUILD_NUMBER 0 CACHE STRING "Build number from the CI, if any. 0 means custom/local build.")

project(Launcher VERSION 6.0.0.${BUILD_NUMBER})
fix_project_version()

enable_testing()

include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_COMMIT)
message(STATUS "Git commit: ${GIT_COMMIT}")
message(STATUS "Git refspec: ${GIT_REFSPEC}")

# Yes, current directory is on the search path of .cpp files
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Automatically handle Qt code generation
set(CMAKE_AUTOMOC ON)

# This means the application does not show up on the dock and does not have a global menu (see cmake/QtCommon.cmake)
set(MACOSX_IS_UIELEMENT 0)


# Output all executables and shared libs in the main build folder, not in subfolders.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
if(UNIX)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})
endif()
set(CMAKE_JAVA_TARGET_OUTPUT_DIR ${PROJECT_BINARY_DIR}/jars)

######## Set compiler flags ########
include(GenerateExportHeader)
set(CMAKE_CXX_FLAGS " -Wall -pedantic -Werror -Wno-deprecated-declarations -Wno-gnu-zero-variadic-macro-arguments -fstack-protector-strong --param=ssp-buffer-size=4 ${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_RELEASE " -O3 -D_FORTIFY_SOURCE=2 ${CMAKE_CXX_FLAGS_RELEASE}")
if(UNIX AND APPLE)
    set(CMAKE_CXX_FLAGS " -stdlib=libc++ ${CMAKE_CXX_FLAGS}")
endif()
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Werror=return-type -O0")

##################################### Set Application options #####################################

######## Set URLs ########
set(Launcher_NEWS_RSS_URL "https://multimc.org/rss.xml" CACHE STRING "URL to fetch Launcher's news RSS feed from.")

######## Set version numbers ########
set(Launcher_VERSION_MAJOR    6)
set(Launcher_VERSION_MINOR    0)
set(Launcher_VERSION_HOTFIX   0)

# Build number
set(Launcher_VERSION_BUILD -1 CACHE STRING "Build number. -1 for no build number.")

# Build platform.
set(Launcher_BUILD_PLATFORM "" CACHE STRING "A short string identifying the platform that this build was built for. Only used by the notification system and to display in the about dialog.")

# Channel list URL
set(Launcher_UPDATER_BASE "" CACHE STRING "Base URL for the updater.")

# Notification URL
set(Launcher_NOTIFICATION_URL "" CACHE STRING "URL for checking for notifications.")

# The metadata server
set(Launcher_META_URL "https://meta.multimc.org/v1/" CACHE STRING "URL to fetch Launcher's meta files from.")

# paste.ee API key
set(Launcher_PASTE_EE_API_KEY "utLvciUouSURFzfjPxLBf5W4ISsUX4pwBDF7N1AfZ" CACHE STRING "API key you can get from paste.ee when you register an account")

# Imgur API Client ID
set(Launcher_IMGUR_CLIENT_ID "5b97b0713fba4a3" CACHE STRING "Client ID you can get from Imgur when you register an application")

# Google analytics ID
set(Launcher_ANALYTICS_ID "UA-87731965-2" CACHE STRING "ID you can get from Google analytics")

# Bug tracker URL
set(Launcher_BUG_TRACKER_URL "" CACHE STRING "URL for the bug tracker.")

# Discord URL
set(Launcher_DISCORD_URL "" CACHE STRING "URL for the Discord guild.")

# Subreddit URL
set(Launcher_SUBREDDIT_URL "" CACHE STRING "URL for the subreddit.")

# Use the secrets library or a public stub?
option(Launcher_EMBED_SECRETS "Determines whether to embed secrets. Secrets are separate and non-public." OFF)

#### Custom target to feed the version to github actions.
add_custom_target(ga_version echo ::set-output name=version::${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}.${PROJECT_VERSION_TWEAK})

################################ 3rd Party Libs ################################

# Find the required Qt parts
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Concurrent Network Test Xml)

####################################### Secrets #######################################

if(Launcher_EMBED_SECRETS)
    add_subdirectory(secrets)
    set(RESOURCE_FOLDER "secrets")
else()
    add_subdirectory(notsecrets)
    set(RESOURCE_FOLDER "notsecrets")
endif()

####################################### Install layout #######################################

set(COMPANY "${Launcher_Copyright}")
set(COPYRIGHT "Copyright 2015-2023 ${Launcher_Copyright}")
set(IDENTIFIER "org.multimc.${Launcher_Name}")
set(DESCRIPTION "${Launcher_Name}:  Minecraft launcher and management utility.")

# How to install the build results
if(UNIX AND APPLE)
    set(BINARY_DEST_DIR "${Launcher_Name}.app/Contents/MacOS")
    set(LIBRARY_DEST_DIR "${Launcher_Name}.app/Contents/MacOS")
    set(JARS_DEST_DIR "${Launcher_Name}.app/Contents/MacOS/jars")

    set(BUNDLE_DEST_DIR ".")

    # Add the icon
    install(FILES ${Launcher_Branding_ICNS} DESTINATION "${Launcher_Name}.app/Contents/Resources" RENAME ${Launcher_Name}.icns)

elseif(UNIX)
    set(BINARY_DEST_DIR "bin")
    set(LIBRARY_DEST_DIR "bin")
    set(JARS_DEST_DIR "bin/jars")

    set(BUNDLE_DEST_DIR ".")

    # Set RPATH
    SET(Launcher_BINARY_RPATH "$ORIGIN/")

    # Install basic runner script
    configure_file(launcher/Launcher.in "${CMAKE_CURRENT_BINARY_DIR}/LauncherScript" @ONLY)
    install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/LauncherScript" DESTINATION ${BUNDLE_DEST_DIR} RENAME ${Launcher_Name})

elseif(WIN32)
    set(BINARY_DEST_DIR ".")
    set(LIBRARY_DEST_DIR ".")
    set(JARS_DEST_DIR "jars")

    set(BUNDLE_DEST_DIR ".")
else()
    message(FATAL_ERROR "Cannot choose a sensible install layout for your platform.")
endif()

################################ Included Libs ################################

include(ExternalProject)
set_directory_properties(PROPERTIES EP_BASE External)

option(NBT_BUILD_SHARED "Build NBT shared library" ON)
option(NBT_USE_ZLIB "Build NBT library with zlib support" OFF)
option(NBT_BUILD_TESTS "Build NBT library tests" OFF) #FIXME: fix unit tests.
set(NBT_NAME Launcher_nbt++)
set(NBT_DEST_DIR ${LIBRARY_DEST_DIR})
add_subdirectory(libraries/libnbtplusplus)

add_subdirectory(libraries/ganalytics) # google analytics library
add_subdirectory(libraries/systeminfo) # system information library
add_subdirectory(libraries/hoedown) # markdown parser
add_subdirectory(libraries/launcher) # java based launcher part for Minecraft
add_subdirectory(libraries/javacheck) # java compatibility checker
add_subdirectory(libraries/xz-embedded) # xz compression
add_subdirectory(libraries/quazip) # zip manipulation library
add_subdirectory(libraries/rainbow) # Qt extension for colors
add_subdirectory(libraries/iconfix) # fork of Qt's QIcon loader
add_subdirectory(libraries/LocalPeer) # fork of a library from Qt solutions
add_subdirectory(libraries/classparser) # google analytics library
add_subdirectory(libraries/optional-bare)
add_subdirectory(libraries/tomlc99) # toml parser
add_subdirectory(libraries/katabasis) # An OAuth2 library that tried to do too much

############################### Built Artifacts ###############################

add_subdirectory(buildconfig)

# NOTE: this must always be last to appease the CMake deity of quirky install command evaluation order.
add_subdirectory(launcher)
