set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM 1)

### translation stuff

file(GLOB PO_TRANSLATION_FILES ${CMAKE_CURRENT_LIST_DIR}/*.po)

## Handle language
foreach(POFILE ${PO_TRANSLATION_FILES})
	string(REGEX REPLACE "\\.po\$" "" LANG "${POFILE}")

	# Gettext lang updater
	add_custom_command(OUTPUT ${POFILE}
		COMMAND msgmerge --lang=${LANG}
		${POFILE} ${CMAKE_CURRENT_LIST_DIR}/messages.pot
		-o ${POFILE}.new
		COMMAND mv ${POFILE}.new ${POFILE}
		DEPENDS ${CMAKE_CURRENT_LIST_DIR}/messages.pot
	)

	# Qt transformer
	add_custom_command(
		OUTPUT ${LANG}.ts
		COMMAND lconvert -locations relative ${POFILE} -o ${LANG}.ts
	)
	list(APPEND QT_TRANSLATION_FILES "${LANG}.ts")
endforeach(POFILE)

qt5_add_translation(TRANSLATION_QM ${QT_TRANSLATION_FILES})
add_custom_target(translations_update ${PO_TRANSLATION_FILES})
add_custom_target(translations DEPENDS ${TRANSLATION_QM})

if(APPLE AND UNIX) ## OSX
	install(FILES ${TRANSLATION_QM} DESTINATION MultiMC.app/Contents/Resources/translations)
else()
	install(FILES ${TRANSLATION_QM} DESTINATION translations)
endif()
