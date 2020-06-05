# What is this?
A simple Fedora package (based on the one for Ubuntu) which includes a script that downloads the latest version of MultiMC into the user's home directory.

When installed it will install a shell script (`run.sh`) to `/opt/MultiMC`, an icon `multimc.svg` to `/usr/share/icons/hicolor/scalable/apps`, and a `.desktop` file to `/usr/share/applications`

# How to build this?
Refer to the Fedora Packaging documentation [here](https://docs.fedoraproject.org/en-US/quick-docs/creating-rpm-packages/#preparing-your-system-to-create-rpm-packages) to initially configure your environment.

Once this environment is configured, run the following within the `multimc` folder

```
fedpkg --release f32 local
```
You can find the release build of the package in the x86_64 or i386 folder, a source RPM will be generated as well.

Replace the release with whichever version of Fedora you are packaging for. 

This package can be built for x86_64 or i686 platforms (32bit Fedora is soon to be EOL so this may be subject to change)
