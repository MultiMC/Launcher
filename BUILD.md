Build Instructions for HaikuOS
==================

# Note

MultiMC is a portable application and is not supposed to be installed into any system folders.
That would be anything outside your home folder. Before running `make install`, make sure
you set the install path to something you have write access to. Never build this under
an administrator/root level account. Don't use `sudo`. It won't work and it's not supposed to work.
Also note that this guide is for development purposes only.

# Branding, identifying marks and API keys

The MultiMC name is a registered trademark. You may not create binary distributions of this code with changes we do not want associated with the name, or without changing the name and branding to your own.

# Getting the source

Clone the source code using git and grab all the submodules:

```
git clone https://github.com/TimofeyLednev/HaikuMC.git
git submodule init
git submodule update
```

# HaikuMC

Getting the project to build and run on Linux is easy if you use any modern and up-to-date linux distribution.

## Build dependencies
* A C++ compiler capable of building C++11 code.
* Qt 5.6+ Development tools (http://qt-project.org/downloads) ("Qt Online Installer for Linux (64 bit)") or the equivalent from your package manager. It is always better to use the Qt from your distribution, as long as it has a new enough version. (for example, `qttools5-dev`)
* cmake 3.1 or newer
* zlib (for example, `zlib1g-dev`)
* Java JDK 8 (for example, `openjdk-8-jdk`)
* GL headers (for example, `libgl1-mesa-dev`)

### Building from command line
You need a source folder, a build folder and an install folder.

Let's say you want everything in `~/HaikuMC/`:

```
# make all the folders
mkdir ~/HaikuMC && cd ~/HaikuMC
mkdir build
mkdir install
# clone the complete source
git clone --recursive https://github.com/TimofeyLednev/HaikuMC.git src
# configure the project
cd build
cmake -DCMAKE_INSTALL_PREFIX=../install ../src
# build & install (use -j with the number of cores your CPU has)
make -j8 install
```

**NOTE:** If you want to treat all warnings as errors add -DCMAKE_BUILD_TYPE=Debug to the above cmake command
