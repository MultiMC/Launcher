project(monocypher LANGUAGES C)

set(monocypher_SOURCES
    include/monocypher.h
    include/monocypher-ed25519.h
    src/monocypher.c
    src/monocypher-ed25519.c
)

add_library(monocypher STATIC ${monocypher_SOURCES})
target_include_directories(monocypher PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
set_property(TARGET monocypher PROPERTY C_STANDARD 99)
