project(Launcher_gui LANGUAGES CXX)

set(GUI_SOURCES
    DesktopServices.h
    DesktopServices.cpp

    # Icons
    icons/LauncherIcon.h
    icons/LauncherIcon.cpp
    icons/IconList.h
    icons/IconList.cpp

    SkinUtils.cpp
    SkinUtils.h
)
################################ COMPILE ################################

add_library(Launcher_gui SHARED ${GUI_SOURCES})
set_target_properties(Launcher_gui PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN 1)

generate_export_header(Launcher_gui)

# Link
target_link_libraries(Launcher_gui Launcher_iconfix Launcher_logic Qt5::Gui)

# Mark and export headers
target_include_directories(Launcher_gui PUBLIC "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}")

# Install it
install(
    TARGETS Launcher_gui
    RUNTIME DESTINATION ${LIBRARY_DEST_DIR}
    LIBRARY DESTINATION ${LIBRARY_DEST_DIR}
)