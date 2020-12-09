## Global variables
%global releaseVer 1.5
%global branch develop

Name:           MultiMC5
Version:        %{branch}
Release:        %{releaseVer}%{?dist}
Summary:        Free, open source launcher and instance manager for Minecraft

License:        ASL 2.0
URL:            https://multimc.org
ExclusiveArch:  %{ix86} x86_64

%undefine _disable_source_fetch
Source0:        https://github.com/MultiMC/MultiMC5/archive/%{branch}.tar.gz
Patch0:         desktopfile.patch

BuildRequires:  fedora-packager fedora-review
Requires:       zenity qt5-qtbase wget
Provides:       multimc MultiMC multimc5

%description
Free, open source launcher and instance manager for Minecraft

%prep
%setup -q

%patch0 -p1

%install
install -dm 755 %{buildroot}/usr/{bin,share/{applications,multimc,icons/hicolor/scalable/apps}}
install -dm 755 %{buildroot}/opt/MultiMC

install -m 0644 application/resources/multimc/scalable/multimc.svg %{buildroot}%{_datadir}/icons/hicolor/scalable/apps/%{name}.svg
install -m 0755 application/package/ubuntu/multimc/opt/multimc/run.sh %{buildroot}%{_datadir}/multimc/run.sh

install -m 0644 application/package/linux/multimc.desktop %{buildroot}%{_datadir}/applications/%{name}.desktop

%files
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/scalable/apps/%{name}.svg
%{_datadir}/multimc
%{_datadir}/multimc/run.sh

%changelog
* Wed Dec 09 11:33:43 EST 2020 Jack Greiner <jack@emoss.org>
- Improvements to build process
- Change install location to be in line with Fedora packaging guidelines.
* Wed Nov 25 22:53:59 CET 2020 kb1000 <fedora@kb1000.de>
- Initial version of the RPM package, based on the Ubuntu package
