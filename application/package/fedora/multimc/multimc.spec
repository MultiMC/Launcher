##Init variables

%global packageVer 1.3
%global _optdir /opt

## Package info declaration

Name:           multimc
Version:        %{packageVer}
Release:        1%{?dist}
Summary:        Free, open source launcher and instance manager for Minecraft

License:        ASL 2.0
URL:            https://multimc.org/
Source0:        %{name}-%{packageVer}.tar.gz

##Builds as noarch, does not install on ARM based machines due to a lack of ARM MultiMC prebuilt binaries.
BuildArch:      noarch
ExclusiveArch:  %{ix86} x86_64 noarch

Requires:       bash
Requires:       java-1.8.0-openjdk
Requires:       qt5
Requires:       zenity
Requires:       zlib

%description
Free, open source launcher and instance manager for Minecraft.

%prep

%setup -q

%install
##Installs directories
install -dm 755 %{buildroot}/usr/{bin,share/{applications,icons/hicolor/scalable/apps}}
install -dm 755 %{buildroot}/opt/MultiMC

##Installs icon and run.sh
install -m 644 %{_builddir}/multimc-%{packageVer}/multimc.svg %{buildroot}/usr/share/icons/hicolor/scalable/apps/multimc.svg
install -m 755 %{_builddir}/multimc-%{packageVer}/run.sh %{buildroot}%{_optdir}/MultiMC/run.sh

##Generates and installs desktop file to /usr/share/applications
cat > %{buildroot}/%{_datadir}/applications/%{name}.desktop << EOF

## Desktop File

[Desktop Entry]
Version=%{packageVer}
Name=MultiMC
GenericName=MultiMC Launcher
Comment=Free, open source launcher and instance manager for Minecraft.
Type=Application
Terminal=false
Exec=%{_optdir}/MultiMC/run.sh
Icon=multimc
Categories=Game
Keywords=game;minecraft;
EOF

%files
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/scalable/apps/multimc.svg
%{_optdir}/MultiMC/
%{_optdir}/MultiMC/run.sh

%changelog
* Thu Jun 18 2020 Jack Greiner <jack@emoss.org> - 1.3-1%{?dist}
- Fixed builds in Copr.
* Mon Jun 8 2020 Jack Greiner <jack@emoss.org> - 1.2-1%{?dist}
- Updated in-line documentation
* Fri Jun 5 2020 Jack Greiner <jack@emoss.org> - 1.0-1%{?dist}
- Updated in-line documentation
* Mon Jun 1 2020 Jack Greiner <jack@emoss.org> - 1.0-1%{?dist}
- Created initial spec file.
