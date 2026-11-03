# Testing the updater on Windows 10

The source tree is portable, but the existing macOS `build` directory is not. Copy the source archive to Windows and configure a new build there.

## Prerequisites

Install:

- Qt 5.15.x with MinGW 64-bit (the repository historically targets MinGW rather than MSVC)
- CMake 3.15 or newer
- MinGW toolchain from the matching Qt installation
- JDK 8 or 11; JDK 21 cannot compile the existing Java 7 launcher helpers
- Git, if you want to initialize the submodules separately

Open **Qt 5.x MinGW Command Prompt** so `qmake`, `mingw32-make`, CMake, and the matching compiler are on `PATH`.

## Configure and build

From the extracted source directory:

```bat
git submodule update --init --recursive
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:\Qt\5.15.x\mingw81_64
cmake --build build --parallel 4
```

If CMake cannot find Java, add the JDK paths explicitly:

```bat
-DJava_JAVA_EXECUTABLE=C:\Program Files\Java\jdk-11\bin\java.exe
-DJava_JAVAC_EXECUTABLE=C:\Program Files\Java\jdk-11\bin\javac.exe
-DJava_JAR_EXECUTABLE=C:\Program Files\Java\jdk-11\bin\jar.exe
```

Run the tests from the same prompt:

```bat
ctest --test-dir build --output-on-failure
```

The new `PackUpdate` and `PackDownload` tests use a localhost mock server. If Windows Firewall asks whether the test executable may accept local connections, allow private-network access. The tests do not need external network access.

## Manual acceptance test

Use a separate MultiMC data directory and a disposable Modrinth instance. Do not test the first update against your only SkyBlock world. The detailed acceptance checklist is in `submission\\pull-request.md` and the updater design/build notes are in `doc\\modpack-updater.md`.

Record the Windows version, compiler/Qt/JDK versions, CMake output, `ctest` result, and whether a real Minecraft launch succeeds. Windows testing complements the macOS evidence; it does not replace the maintainer review requested in the Discord discussion.
