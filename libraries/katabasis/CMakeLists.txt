cmake_minimum_required(VERSION 3.6)

project(Katabasis)
enable_testing()

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD_REQUIRED true)
set(CMAKE_C_STANDARD_REQUIRED true)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

find_package(Qt6 COMPONENTS Core Network REQUIRED)

set( katabasis_PRIVATE
    src/DeviceFlow.cpp
    src/JsonResponse.cpp
    src/JsonResponse.h
    src/PollServer.cpp
    src/Reply.cpp
)

set( katabasis_PUBLIC
    include/katabasis/DeviceFlow.h
    include/katabasis/Globals.h
    include/katabasis/PollServer.h
    include/katabasis/Reply.h
    include/katabasis/RequestParameter.h
)

add_library( Katabasis STATIC ${katabasis_PRIVATE} ${katabasis_PUBLIC} )
target_link_libraries(Katabasis Qt6::Core Qt6::Network)

# needed for statically linked Katabasis in shared libs on x86_64
set_target_properties(Katabasis
    PROPERTIES POSITION_INDEPENDENT_CODE TRUE
)

target_include_directories(Katabasis PUBLIC include PRIVATE src include/katabasis)
