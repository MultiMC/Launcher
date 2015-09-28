set -e

if [ "$TRAVIS_OS_NAME" = "linux" ]
then
	QT_WITHOUT_DOTS=$(echo $QT_VERSION | grep -oP "[^\.]*" | tr -d '\n' | tr '[:upper:]' '[:lower]')
	QT_PKG_PREFIX=$(echo $QT_WITHOUT_DOTS | cut -c1-4)
	echo $QT_WITHOUT_DOTS
	echo $QT_PKG_PREFIX
	#sudo add-apt-repository -y ppa:beineri/opt-${QT_WITHOUT_DOTS}
	wget http://static.02jandal.xyz/qt${QT_WITHOUT_DOTS}-precise_${QT_VERSION}-1_amd64.deb
	sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test # for a recent GCC
	sudo add-apt-repository "deb http://llvm.org/apt/precise/ llvm-toolchain-precise-3.7 main"

	sudo apt-get update -qq
	#sudo apt-get install ${QT_PKG_PREFIX}base ${QT_PKG_PREFIX}svg ${QT_PKG_PREFIX}tools ${QT_PKG_PREFIX}x11extras ${QT_PKG_PREFIX}webengine
	sudo dpkg -i qt${QT_WITHOUT_DOTS}-precise_${QT_VERSION}-1_amd64.deb

	sudo mkdir -p /opt/cmake-3/
	wget http://www.cmake.org/files/v3.2/cmake-3.2.2-Linux-x86_64.sh
	sudo sh cmake-3.2.2-Linux-x86_64.sh --skip-license --prefix=/opt/cmake-3/

	export CMAKE_PREFIX_PATH=/usr/local/Qt-${QT_VERSION}/lib/cmake
	export PATH=/opt/cmake-3/bin:/usr/local/Qt-${QT_VERSION}/bin:$PATH

	if [ "$CXX" = "g++" ]; then
		sudo apt-get install -y -qq g++-5
		export CXX='g++-5' CC='gcc-5'
	fi
	if [ "$CXX" = "clang++" ]; then
		wget -O - http://llvm.org/apt/llvm-snapshot.gpg.key|sudo apt-key add -
		sudo apt-get install -y -qq clang-3.7 liblldb-3.7 libclang1-3.7 libllvm3.7 lldb-3.7 llvm-3.7 llvm-3.7-runtime
		export CXX='clang++-3.7' CC='clang-3.7'
	fi
else
	brew update
	brew install qt5
	brew install cmake
	export CMAKE_PREFIX_PATH=/usr/local/lib/cmake
fi

# Output versions
cmake -version
qmake -version
$CXX -v
