set(MULTIMC_LOGIC_SRCS

# Base classes and infrastructure
 logic/BaseVersion.h
 logic/MinecraftVersion.h
 logic/InstanceFactory.h
 logic/InstanceFactory.cpp
 logic/BaseInstance.h
 logic/BaseInstance.cpp
 logic/BaseInstance_p.h

 logic/MinecraftProcess.h
 logic/MinecraftProcess.cpp
 logic/Mod.h
 logic/Mod.cpp
 logic/ModList.h
 logic/ModList.cpp

# Basic instance launcher for starting from terminal
 logic/InstanceLauncher.h
 logic/InstanceLauncher.cpp

# JSON parsing helpers
 logic/MMCJson.h
 logic/MMCJson.cpp

# Network stuffs
 logic/net/NetAction.h
 logic/net/MD5EtagDownload.h
 logic/net/MD5EtagDownload.cpp
 logic/net/ByteArrayDownload.h
 logic/net/ByteArrayDownload.cpp
 logic/net/CacheDownload.h
 logic/net/CacheDownload.cpp
 logic/net/ForgeMirrors.h
 logic/net/ForgeMirrors.cpp
 logic/net/ForgeXzDownload.h
 logic/net/ForgeXzDownload.cpp
 logic/net/NetJob.h
 logic/net/NetJob.cpp
 logic/net/HttpMetaCache.h
 logic/net/HttpMetaCache.cpp
 logic/net/PasteUpload.h
 logic/net/PasteUpload.cpp
 logic/net/URLConstants.h
 logic/net/URLConstants.cpp

# Yggdrasil login stuff
 logic/auth/AuthSession.h
 logic/auth/AuthSession.cpp
 logic/auth/MojangAccountList.h
 logic/auth/MojangAccountList.cpp
 logic/auth/MojangAccount.h
 logic/auth/MojangAccount.cpp
 logic/auth/YggdrasilTask.h
 logic/auth/YggdrasilTask.cpp
 logic/auth/flows/AuthenticateTask.h
 logic/auth/flows/AuthenticateTask.cpp
 logic/auth/flows/RefreshTask.cpp
 logic/auth/flows/RefreshTask.cpp
 logic/auth/flows/ValidateTask.h
 logic/auth/flows/ValidateTask.cpp

# Update system
 logic/updater/UpdateChecker.h
 logic/updater/UpdateChecker.cpp
 logic/updater/DownloadUpdateTask.h
 logic/updater/DownloadUpdateTask.cpp
 logic/updater/NotificationChecker.h
 logic/updater/NotificationChecker.cpp

# News System
 logic/news/NewsChecker.h
 logic/news/NewsChecker.cpp
 logic/news/NewsEntry.h
 logic/news/NewsEntry.cpp

# Status system
 logic/status/StatusChecker.h
 logic/status/StatusChecker.cpp

# Legacy instances
 logic/LegacyInstance.h
 logic/LegacyInstance.cpp
 logic/LegacyInstance_p.h
 logic/LegacyUpdate.h
 logic/LegacyUpdate.cpp

 logic/LegacyForge.h
 logic/LegacyForge.cpp

# OneSix instances
 logic/OneSixUpdate.h
 logic/OneSixUpdate.cpp
 logic/OneSixInstance.h
 logic/OneSixInstance.cpp
 logic/OneSixInstance_p.h

# OneSix version json infrastructure
 logic/OneSixVersionBuilder.h
 logic/OneSixVersionBuilder.cpp
 logic/VersionFile.h
 logic/VersionFile.cpp
 logic/VersionFinal.h
 logic/VersionFinal.cpp
 logic/OneSixLibrary.h
 logic/OneSixLibrary.cpp
 logic/OneSixRule.h
 logic/OneSixRule.cpp
 logic/OpSys.h
 logic/OpSys.cpp

# Mod installers
 logic/BaseInstaller.h
 logic/BaseInstaller.cpp
 logic/ForgeInstaller.h
 logic/ForgeInstaller.cpp
 logic/LiteLoaderInstaller.h
 logic/LiteLoaderInstaller.cpp

# Nostalgia
 logic/NostalgiaInstance.h
 logic/NostalgiaInstance.cpp

# FTB
 logic/OneSixFTBInstance.h
 logic/OneSixFTBInstance.cpp
 logic/LegacyFTBInstance.h
 logic/LegacyFTBInstance.cpp

# Lists
 logic/lists/InstanceList.h
 logic/lists/InstanceList.cpp
 logic/lists/BaseVersionList.h
 logic/lists/BaseVersionList.cpp
 logic/lists/MinecraftVersionList.h
 logic/lists/MinecraftVersionList.cpp
 logic/lists/LwjglVersionList.h
 logic/lists/LwjglVersionList.cpp
 logic/lists/ForgeVersionList.h
 logic/lists/ForgeVersionList.cpp
 logic/lists/JavaVersionList.h
 logic/lists/JavaVersionList.cpp
 logic/lists/LiteLoaderVersionList.h
 logic/lists/LiteLoaderVersionList.cpp

# The screenshots feature
 logic/screenshots/Screenshot.h
 logic/screenshots/Screenshot.cpp
 logic/screenshots/ScreenshotList.h
 logic/screenshots/ScreenshotList.cpp
 logic/screenshots/ImgurUpload.h
 logic/screenshots/ImgurUpload.cpp
 logic/screenshots/ImgurAlbumCreation.h
 logic/screenshots/ImgurAlbumCreation.cpp

# Icons
 logic/icons/MMCIcon.h
 logic/icons/MMCIcon.cpp
 logic/icons/IconList.h
 logic/icons/IconList.cpp


# Misc model/view
 logic/EnabledItemFilter.h
 logic/EnabledItemFilter.cpp

# Tasks
 logic/tasks/ProgressProvider.h
 logic/tasks/Task.h
 logic/tasks/Task.cpp
 logic/tasks/ThreadTask.h
 logic/tasks/ThreadTask.cpp
 logic/tasks/SequentialTask.h
 logic/tasks/SequentialTask.cpp

# Utilities
 logic/JavaChecker.h
 logic/JavaChecker.cpp
 logic/JavaUtils.h
 logic/JavaUtils.cpp
 logic/NagUtils.h
 logic/NagUtils.cpp
 logic/SkinUtils.h
 logic/SkinUtils.cpp
 logic/JavaCheckerJob.h
 logic/JavaCheckerJob.cpp

# Assets
 logic/assets/AssetsMigrateTask.h
 logic/assets/AssetsMigrateTask.cpp
 logic/assets/AssetsUtils.h
 logic/assets/AssetsUtils.cpp

# Tools
 logic/tools/BaseExternalTool.h
 logic/tools/BaseExternalTool.cpp
 logic/tools/MCEditTool.h
 logic/tools/MCEditTool.cpp
 logic/tools/BaseProfiler.h
 logic/tools/BaseProfiler.cpp
 logic/tools/JProfiler.h
 logic/tools/JProfiler.cpp
 logic/tools/JVisualVM.h
 logic/tools/JVisualVM.cpp
 
 PARENT_SCOPE
)
