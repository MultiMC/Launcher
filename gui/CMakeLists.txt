####### X11 Stuff #######
if(UNIX AND NOT APPLE)
    set(MULTIMC_GUI_SRCS_platform_dependent gui/Platform_X11.cpp)
else()
    set(MULTIMC_GUI_SRCS_platform_dependent gui/Platform_Other.cpp)
endif()

set(MULTIMC_GUI_SRCS

# Windows
 gui/MainWindow.h
 gui/MainWindow.cpp
 gui/ConsoleWindow.h
 gui/ConsoleWindow.cpp

# Dialogs
 gui/dialogs/SettingsDialog.h
 gui/dialogs/SettingsDialog.cpp
 gui/dialogs/CopyInstanceDialog.h
 gui/dialogs/CopyInstanceDialog.cpp
 gui/dialogs/NewInstanceDialog.cpp
 gui/dialogs/ProgressDialog.h
 gui/dialogs/ProgressDialog.cpp
 gui/dialogs/AboutDialog.h
 gui/dialogs/AboutDialog.cpp
 gui/dialogs/VersionSelectDialog.h
 gui/dialogs/VersionSelectDialog.cpp
 gui/dialogs/LwjglSelectDialog.h
 gui/dialogs/LwjglSelectDialog.cpp
 gui/dialogs/InstanceSettings.h
 gui/dialogs/InstanceSettings.cpp
 gui/dialogs/IconPickerDialog.h
 gui/dialogs/IconPickerDialog.cpp
 gui/dialogs/LegacyModEditDialog.h
 gui/dialogs/LegacyModEditDialog.cpp
 gui/dialogs/OneSixModEditDialog.h
 gui/dialogs/OneSixModEditDialog.cpp
 gui/dialogs/ModEditDialogCommon.h
 gui/dialogs/ModEditDialogCommon.cpp
 gui/dialogs/EditNotesDialog.h
 gui/dialogs/EditNotesDialog.cpp
 gui/dialogs/CustomMessageBox.h
 gui/dialogs/CustomMessageBox.cpp
 gui/dialogs/EditAccountDialog.h
 gui/dialogs/EditAccountDialog.cpp
 gui/dialogs/AccountListDialog.h
 gui/dialogs/AccountListDialog.cpp
 gui/dialogs/AccountSelectDialog.h
 gui/dialogs/AccountSelectDialog.cpp
 gui/dialogs/UpdateDialog.h
 gui/dialogs/UpdateDialog.cpp
 gui/dialogs/ScreenshotDialog.h
 gui/dialogs/ScreenshotDialog.cpp
 gui/dialogs/NotificationDialog.h
 gui/dialogs/NotificationDialog.cpp

# Widgets
 gui/widgets/Common.h
 gui/widgets/Common.cpp
 gui/widgets/ModListView.h
 gui/widgets/ModListView.cpp
 gui/widgets/VersionListView.h
 gui/widgets/VersionListView.cpp
 gui/widgets/LabeledToolButton.h
 gui/widgets/LabeledToolButton.cpp
 gui/widgets/MCModInfoFrame.h
 gui/widgets/MCModInfoFrame.cpp

# Instance group view
 gui/groupview/Group.cpp
 gui/groupview/Group.h
 gui/groupview/GroupedProxyModel.cpp
 gui/groupview/GroupedProxyModel.h
 gui/groupview/GroupView.cpp
 gui/groupview/GroupView.h
 gui/groupview/InstanceDelegate.cpp
 gui/groupview/InstanceDelegate.h

 ${MULTIMC_GUI_SRCS_platform_dependent}

 PARENT_SCOPE
)

######## UIs ########
set(MULTIMC_GUI_UIS

# Windows
 gui/MainWindow.ui
 gui/ConsoleWindow.ui

# Dialogs
 gui/dialogs/SettingsDialog.ui
 gui/dialogs/CopyInstanceDialog.ui
 gui/dialogs/NewInstanceDialog.ui
 gui/dialogs/AboutDialog.ui
 gui/dialogs/VersionSelectDialog.ui
 gui/dialogs/LwjglSelectDialog.ui
 gui/dialogs/InstanceSettings.ui
 gui/dialogs/ProgressDialog.ui
 gui/dialogs/IconPickerDialog.ui
 gui/dialogs/LegacyModEditDialog.ui
 gui/dialogs/OneSixModEditDialog.ui
 gui/dialogs/EditNotesDialog.ui
 gui/dialogs/AccountListDialog.ui
 gui/dialogs/AccountSelectDialog.ui
 gui/dialogs/EditAccountDialog.ui
 gui/dialogs/UpdateDialog.ui
 gui/dialogs/ScreenshotDialog.ui
 gui/dialogs/NotificationDialog.ui

# Widgets/other
 gui/widgets/MCModInfoFrame.ui

 PARENT_SCOPE
)
