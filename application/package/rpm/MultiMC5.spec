## Global variables
%global releaseVer 1.5

Name:           MultiMC5
Version:        0
Release:        %{releaseVer}%{?dist}
Summary:        Free, open source launcher and instance manager for Minecraft

License:        ASL 2.0
URL:            https://multimc.org
ExclusiveArch:  %{ix86} x86_64

BuildRequires:  rpm-build
Patch0:         desktopfile.patch

Requires:       zenity qt5-qtbase wget
Recommends:     java-latest-openjdk
Provides:       multimc MultiMC multimc5

%description
Free, open source launcher and instance manager for Minecraft

%prep
cp ../ubuntu/multimc/opt/multimc/run.sh %{_builddir}/run.sh
cp ../../resources/multimc/scalable/multimc.svg %{_builddir}/multimc.svg

%build
cat > %{_builddir}/multimc.desktop << EOF

## Desktop File

[Desktop Entry]
Version=%{version}-%{release}
Name=MultiMC
GenericName=MultiMC Launcher
Comment=Free, open source launcher and instance manager for Minecraft.
Type=Application
Terminal=false
Exec=%{_datadir}/multimc/run.sh
Icon=multimc
Categories=Game
Keywords=game;minecraft;
EOF

%install
install -dm 755 %{buildroot}/usr/{bin,share/{applications,multimc,icons/hicolor/scalable/apps}}
install -dm 755 %{buildroot}/opt/MultiMC

install -m 0644 multimc.svg %{buildroot}%{_datadir}/icons/hicolor/scalable/apps/multimc.svg
install -m 0755 run.sh %{buildroot}%{_datadir}/multimc/run.sh

install -m 0644 multimc.desktop %{buildroot}%{_datadir}/applications/multimc.desktop

%files
%{_datadir}/applications/multimc.desktop
%{_datadir}/icons/hicolor/scalable/apps/multimc.svg
%{_datadir}/multimc/run.sh

%clean
cp -r %{_rpmdir}/* %{_builddir}
rm %{_builddir}/run.sh
rm %{_builddir}/multimc.svg
rm %{_builddir}/multimc.desktop

%changelog
* Wed Dec 09 11:33:43 EST 2020 Jack Greiner <jack@emoss.org>
- Improvements to build process, embed desktop file into SPEC to dynamically generate.
- Change install location to be in line with Fedora packaging guidelines.
* Wed Nov 25 22:53:59 CET 2020 kb1000 <fedora@kb1000.de>
- Initial version of the RPM package, based on the Ubuntu package
