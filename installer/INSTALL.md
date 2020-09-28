Making Installer
================

# Contents
* [Note](#note)
* [Getting NSIS](#NSIS)
* [Generating Installer](#installer)

# Note
NSIS (Nullsoft Scriptable Install System) is a professional open source system to create Windows installers. It is designed to be as small and flexible as possible and is therefore very suitable for internet distribution

# Gettings NSIS
NSIS binaries and source code are available from the NSIS SourceForge Page (https://nsis.sourceforge.io/Download)

# Generating Installer
Copy Installer.nsi, MultiMC.ico, and license.txt to the root of a MultiMC build. Launch NSIS and select "Compile NSI Scripts". Select Installer.nsi and NSIS will create the installer MultiMC-<version>.exe in the root of the MultiMC build.