project(MultiMC-Logic)

set(LOGIC_SOURCES
	# LOGIC - Base classes and infrastructure
	BaseVersionList.h
	BaseVersionList.cpp
	InstanceList.h
	InstanceList.cpp
	BaseVersion.h
	BaseProcess.h
	BaseProcess.cpp
	BaseInstance.h
	BaseInstance.cpp
	FileStore.h
	FileStore.cpp
	NullInstance.h
	MMCZip.h
	MMCZip.cpp

	# Various utility classes
	Json.h
	Json.cpp
	Exception.h
	FileSystem.h
	FileSystem.cpp
	Functional.h
	MMCStrings.h
	MMCStrings.cpp
	BaseConfigObject.h
	BaseConfigObject.cpp
	AbstractCommonModel.h
	AbstractCommonModel.cpp

	# Prefix tree where node names are strings between separators
	SeparatorPrefixTree.h

	# WARNING: globals live here
	Env.h
	Env.cpp

	# RW lock protected map
	RWStorage.h

	# A variable that has an implicit default value and keeps track of changes
	DefaultVariable.h

	# a smart pointer wrapper intended for safer use with Qt signal/slot mechanisms
	QObjectPtr.h

	# Wonko stuff
	wonko/BaseResource.h
	wonko/BaseResource.cpp
	wonko/DownloadableResource.cpp
	wonko/DownloadableResource.h
	wonko/WonkoPackage.h
	wonko/WonkoPackage.cpp
	wonko/WonkoPackageVersion.h
	wonko/WonkoPackageVersion.cpp
	wonko/Rules.h
	wonko/Rules.cpp
	wonko/OpSys.h
	wonko/OpSys.cpp

	# Resources
	resources/IconResourceHandler.cpp
	resources/IconResourceHandler.h
	resources/Resource.cpp
	resources/Resource.h
	resources/ResourceHandler.cpp
	resources/ResourceHandler.h
	resources/ResourceObserver.cpp
	resources/ResourceObserver.h
	resources/WebResourceHandler.cpp
	resources/WebResourceHandler.h
	resources/ResourceProxyModel.h
	resources/ResourceProxyModel.cpp

	# network stuffs
	net/NetAction.h
	net/MD5EtagDownload.h
	net/MD5EtagDownload.cpp
	net/ByteArrayDownload.h
	net/ByteArrayDownload.cpp
	net/CacheDownload.h
	net/CacheDownload.cpp
	net/NetJob.h
	net/NetJob.cpp
	net/HttpMetaCache.h
	net/HttpMetaCache.cpp
	net/PasteUpload.h
	net/PasteUpload.cpp
	net/URLConstants.h
	net/URLConstants.cpp

	# Accounts stuff
	auth/AccountModel.h
	auth/AccountModel.cpp
	auth/BaseAccount.h
	auth/BaseAccount.cpp
	auth/BaseSession.h
	auth/BaseSession.cpp
	auth/BaseAccountType.h

	# Imgur accounts
	#auth/imgur/ImgurAccount.h
	#auth/imgur/ImgurAccount.cpp
	#auth/imgur/ImgurAuthTask.h
	#auth/imgur/ImgurAuthTask.cpp

	# Mojang (yggdrasil) accounts
	auth/minecraft/MojangAuthSession.h
	auth/minecraft/MojangAuthSession.cpp
	auth/minecraft/MojangAccount.h
	auth/minecraft/MojangAccount.cpp
	auth/minecraft/tasks/YggdrasilTask.h
	auth/minecraft/tasks/YggdrasilTask.cpp
	auth/minecraft/tasks/AuthenticateTask.h
	auth/minecraft/tasks/AuthenticateTask.cpp
	auth/minecraft/tasks/RefreshTask.cpp
	auth/minecraft/tasks/RefreshTask.cpp
	auth/minecraft/tasks/ValidateTask.h
	auth/minecraft/tasks/ValidateTask.cpp

	# Update system
	updater/GoUpdate.h
	updater/GoUpdate.cpp
	updater/UpdateChecker.h
	updater/UpdateChecker.cpp
	updater/DownloadTask.h
	updater/DownloadTask.cpp

	# Notifications - short warning messages
	notifications/NotificationChecker.h
	notifications/NotificationChecker.cpp

	# News System
	news/NewsChecker.h
	news/NewsChecker.cpp
	news/NewsEntry.h
	news/NewsEntry.cpp

	# Status system
	status/StatusChecker.h
	status/StatusChecker.cpp

	# Minecraft - OneSix support
	minecraft/onesix/OneSixFormat.h
	minecraft/onesix/OneSixRead.cpp
	minecraft/onesix/OneSixWrite.cpp
	minecraft/onesix/OneSixUtils.cpp
	minecraft/onesix/OneSixProfileStrategy.cpp
	minecraft/onesix/OneSixProfileStrategy.h
	minecraft/onesix/OneSixUpdate.h
	minecraft/onesix/OneSixUpdate.cpp
	minecraft/onesix/OneSixInstance.h
	minecraft/onesix/OneSixInstance.cpp

	# Minecraft - Legacy support
	minecraft/legacy/LegacyInstance.cpp
	minecraft/legacy/LegacyInstance.h
	minecraft/legacy/LegacyUpdate.cpp
	minecraft/legacy/LegacyUpdate.h
	minecraft/legacy/LwjglVersionList.h
	minecraft/legacy/LwjglVersionList.cpp

	# FTB
	minecraft/ftb/FTBInstance.h
	minecraft/ftb/FTBInstance.cpp
	minecraft/ftb/FTBProfileStrategy.h
	minecraft/ftb/FTBProfileStrategy.cpp
	minecraft/ftb/FTBPlugin.h
	minecraft/ftb/FTBPlugin.cpp

	# Minecraft - Wonko
	minecraft/wonko/WonkoFormat.h
	minecraft/wonko/WonkoRead.cpp

	# Minecrafr resource types
	minecraft/Assets.h
	minecraft/Assets.cpp
	minecraft/Libraries.h
	minecraft/Libraries.cpp

	minecraft/SkinUtils.h
	minecraft/SkinUtils.cpp
	minecraft/GradleSpecifier.h
	minecraft/JarMod.h
	minecraft/MinecraftInstance.cpp
	minecraft/MinecraftInstance.h
	minecraft/Process.cpp
	minecraft/Process.h
	minecraft/MinecraftProfile.cpp
	minecraft/MinecraftProfile.h
	minecraft/Patch.cpp
	minecraft/Patch.h
	minecraft/Resources.h
	minecraft/NullProfileStrategy.h
	minecraft/ProfileUtils.cpp
	minecraft/ProfileUtils.h
	minecraft/ProfileStrategy.h
	minecraft/Library.cpp
	minecraft/Library.h
	minecraft/VersionBuildError.h
	minecraft/Package.h
	minecraft/VersionFilterData.h
	minecraft/VersionFilterData.cpp
	minecraft/Mod.h
	minecraft/Mod.cpp
	minecraft/ModList.h
	minecraft/ModList.cpp

	# A Recursive file system watcher
	RecursiveFileSystemWatcher.h
	RecursiveFileSystemWatcher.cpp

	# the screenshots feature
	screenshots/Screenshot.h
	screenshots/ImgurUpload.h
	screenshots/ImgurUpload.cpp
	screenshots/ImgurAlbumCreation.h
	screenshots/ImgurAlbumCreation.cpp

	# Icons
	icons/MMCIcon.h
	icons/MMCIcon.cpp
	icons/IconList.h
	icons/IconList.cpp

	# Tasks
	tasks/Task.h
	tasks/Task.cpp
	tasks/ThreadTask.h
	tasks/ThreadTask.cpp
	tasks/SequentialTask.h
	tasks/SequentialTask.cpp
	tasks/StandardTask.h
	tasks/StandardTask.cpp
	tasks/MoveFileTask.h
	tasks/MoveFileTask.cpp
	tasks/CopyFileTask.h
	tasks/CopyFileTask.cpp

	# Settings
	settings/INIFile.cpp
	settings/INIFile.h
	settings/INISettingsObject.cpp
	settings/INISettingsObject.h
	settings/OverrideSetting.cpp
	settings/OverrideSetting.h
	settings/Setting.cpp
	settings/Setting.h
	settings/SettingsObject.cpp
	settings/SettingsObject.h

	# Java related code
	java/JavaChecker.h
	java/JavaChecker.cpp
	java/JavaUtils.h
	java/JavaUtils.cpp
	java/JavaVersionList.h
	java/JavaVersionList.cpp
	java/JavaCheckerJob.h
	java/JavaCheckerJob.cpp

	# Translations
	trans/TranslationDownloader.h
	trans/TranslationDownloader.cpp

	# Tools
	tools/BaseExternalTool.cpp
	tools/BaseExternalTool.h
	tools/BaseProfiler.cpp
	tools/BaseProfiler.h
	tools/JProfiler.cpp
	tools/JProfiler.h
	tools/JVisualVM.cpp
	tools/JVisualVM.h
	tools/MCEditTool.cpp
	tools/MCEditTool.h

)
################################ COMPILE ################################

# Add common library
add_library(MultiMC_logic STATIC ${LOGIC_SOURCES})

# Link
target_link_libraries(MultiMC_logic iconfix libUtil LogicalGui ${QUAZIP_LIBRARIES}
	Qt5::Core Qt5::Xml Qt5::Widgets Qt5::Network Qt5::Concurrent
		${MultiMC_LINK_ADDITIONAL_LIBS})

add_dependencies(MultiMC_logic QuaZIP)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
