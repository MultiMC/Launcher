find_package(Doxygen)

if(DOXYGEN_FOUND)
    set(MultiMC_DOX_DESCRIPTION "MultiMC Minecraft launcher and management utility.")
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
    add_custom_target(MultiMC_GENERATE_DOXYGEN
     COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
     WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
     COMMENT "Generating documentation using doxygen..."
     VERBATIM
    )
endif(DOXYGEN_FOUND)
