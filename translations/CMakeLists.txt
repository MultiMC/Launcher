set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM 1)

### translation stuff

file(GLOB PO_TRANSLATION_FILES ${CMAKE_CURRENT_LIST_DIR}/*.po)
add_custom_target(translations_update)

foreach(FILE ${FILES_TO_TRANSLATE})
	if(NOT "${FILE}" MATCHES "\\.ui\$")
		file(RELATIVE_PATH RELFILE "${PROJECT_SOURCE_DIR}" "${FILE}")
		list(APPEND CPP_FILES_TO_TRANSLATE "${RELFILE}")
	endif(NOT "${FILE}" MATCHES "\\.ui\$")
endforeach(FILE)

add_custom_command(TARGET translations_update
	COMMAND xgettext -k -ktr 
	-o ${CMAKE_CURRENT_LIST_DIR}/messages.pot
	-D .
	${CPP_FILES_TO_TRANSLATE}
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

## Handle language
foreach(POFILE ${PO_TRANSLATION_FILES})
	string(REGEX REPLACE "\\.po\$" "" LANG "${POFILE}")

	# Gettext lang updater
	add_custom_command(TARGET translations_update
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
add_custom_target(translations DEPENDS ${TRANSLATION_QM})

if(APPLE AND UNIX) ## OSX
	install(FILES ${TRANSLATION_QM} DESTINATION MultiMC.app/Contents/Resources/translations)
else()
	install(FILES ${TRANSLATION_QM} DESTINATION translations)
endif()
