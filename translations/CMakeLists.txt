set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM 1)

### translation stuff

file(GLOB PO_TRANSLATION_FILES ${CMAKE_CURRENT_LIST_DIR}/*.po)

foreach(POFILE ${PO_TRANSLATION_FILES})
	string(REGEX REPLACE "\\.po\$" "" TSFILE "${POFILE}")
	add_custom_command(
		OUTPUT ${TSFILE}.ts
		COMMAND lconvert -locations relative ${POFILE} -o ${TSFILE}.ts
	)
	list(APPEND QT_TRANSLATION_FILES "${TSFILE}.ts")
endforeach(POFILE)

#Is this still needed?
#qt5_create_translation(TRANSLATION_MESSAGES ${FILES_TO_TRANSLATE} ${QT_TRANSLATION_FILES})
qt5_add_translation(TRANSLATION_QM ${QT_TRANSLATION_FILES})
add_custom_target(translations_update DEPENDS ${TRANSLATION_MESSAGES})
add_custom_target(translations DEPENDS ${TRANSLATION_QM})

if(APPLE AND UNIX) ## OSX
	install(FILES ${TRANSLATION_QM} DESTINATION MultiMC.app/Contents/Resources/translations)
else()
	install(FILES ${TRANSLATION_QM} DESTINATION translations)
endif()
