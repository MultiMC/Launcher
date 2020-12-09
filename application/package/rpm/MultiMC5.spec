## Global variables
%global packageVer 1.5

Name:           MultiMC5
Version:        %{packageVer}
Release:        1%{?dist}
Summary:        Free, open source launcher and instance manager for Minecraft

License:        ASL 2.0
URL:            https://multimc.org
ExclusiveArch:  %{ix86} x86_64

%undefine _disable_source_fetch
Source0:        https://github.com/MultiMC/MultiMC5/archive/develop.tar.gz

BuildRequires:  fedora-packager fedora-review
Requires:       zenity qt5-qtbase wget
Provides:       multimc MultiMC multimc5

%description
Free, open source launcher and instance manager for Minecraft

%prep


%build


%install
mkdir -p %{buildroot}/opt/multimc
install -m 0644 ../ubuntu/multimc/opt/multimc/icon.svg %{buildroot}/opt/multimc/icon.svg
install -m 0755 ../ubuntu/multimc/opt/multimc/run.sh %{buildroot}/opt/multimc/run.sh
mkdir -p %{buildroot}/%{_datadir}/applications
install -m 0644 ../ubuntu/multimc/usr/share/applications/multimc.desktop %{buildroot}/%{_datadir}/applications/multimc.desktop


%files
%dir /opt/multimc
/opt/multimc/icon.svg
/opt/multimc/run.sh
%{_datadir}/applications/multimc.desktop



%changelog
* Wed Nov 25 22:53:59 CET 2020 kb1000 <fedora@kb1000.de>
- Initial version of the RPM package, based on the Ubuntu package
