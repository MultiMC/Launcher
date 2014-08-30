set_directory_properties(PROPERTIES CLEAN_NO_CUSTOM 1)

### translation stuff

file(GLOB PO_TRANSLATION_FILES ${CMAKE_CURRENT_LIST_DIR}/*.po)
add_custom_target(translations_update)


## QT Update Logic
set(LSTFILE "${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/base_lst_file")
set(LSTFILEBUF)
foreach(FILE ${FILES_TO_TRANSLATE})
	set(LSTFILEBUF "${FILE}\n${LSTFILEBUF}")
endforeach(FILE)
file(WRITE ${LSTFILE} "${LSTFILEBUF}")

add_custom_command(TARGET translations_update
	COMMAND lconvert -locations relative ${CMAKE_CURRENT_LIST_DIR}/messages.pot 
	-o ${CMAKE_CURRENT_LIST_DIR}/base.ts
	WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
	DEPENDS ${PROJECT_SOURCE_DIR}
)

add_custom_command(TARGET translations_update
	COMMAND lupdate-qt5
	"@${LSTFILE}"
	-ts ${CMAKE_CURRENT_LIST_DIR}/base.ts
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
	DEPENDS ${FILES_TO_TRANSLATE} ${LSTFILE}
)

add_custom_command(TARGET translations_update
	COMMAND lconvert -locations relative ${CMAKE_CURRENT_LIST_DIR}/base.ts 
	-o ${CMAKE_CURRENT_LIST_DIR}/messages.pot
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
	DEPENDS ${FILES_TO_TRANSLATE}
)


## Handle language
foreach(POFILE ${PO_TRANSLATION_FILES})
	file(RELATIVE_PATH RELFILE "${PROJECT_SOURCE_DIR}" "${POFILE}")
	string(REGEX REPLACE "\\.po\$" "" LANG "${RELFILE}")

	# Gettext lang updater
	add_custom_command(TARGET translations_update
		COMMAND msgmerge --lang=${LANG}
		${POFILE} ${CMAKE_CURRENT_LIST_DIR}/messages.pot
		--force-po
		-o ${POFILE}.new
		-D ${PROJECT_SOURCE_DIR}
		COMMAND mv ${POFILE}.new ${POFILE}
		WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
		DEPENDS ${CMAKE_CURRENT_LIST_DIR}/messages.pot
	)

	# Qt transformer
	add_custom_command(
		OUTPUT "mmc_${LANG}.ts"
		COMMAND lconvert -locations relative ${POFILE} -o ${LANG}.ts
	)
	list(APPEND QT_TRANSLATION_FILES "mmc_${LANG}.ts")
endforeach(POFILE)

qt5_add_translation(TRANSLATION_QM ${QT_TRANSLATION_FILES})
add_custom_target(translations DEPENDS ${TRANSLATION_QM})

if(APPLE AND UNIX) ## OSX
	install(FILES ${TRANSLATION_QM} DESTINATION MultiMC.app/Contents/Resources/translations)
else()
	install(FILES ${TRANSLATION_QM} DESTINATION translations)
endif()
