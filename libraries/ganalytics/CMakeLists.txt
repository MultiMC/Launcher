project(ganalytics)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Network)

set(ganalytics_SOURCES
src/ganalytics.cpp
src/ganalytics_worker.cpp
src/ganalytics_worker.h
include/ganalytics.h
)

add_library(ganalytics STATIC ${ganalytics_SOURCES})
target_link_libraries(ganalytics Qt6::Core Qt6::Gui Qt6::Network)
target_include_directories(ganalytics PUBLIC include)
target_link_libraries(ganalytics systeminfo)
