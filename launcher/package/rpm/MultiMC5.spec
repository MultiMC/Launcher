Name:           MultiMC5
Version:        1.4
Release:        3%{?dist}
Summary:        A local install wrapper for MultiMC

License:        ASL 2.0
URL:            https://multimc.org
BuildArch:      x86_64

Requires:       zenity qt5-qtbase wget xrandr
Provides:       multimc MultiMC multimc5

%description
A local install wrapper for MultiMC

%prep


%build

%install
mkdir -p %{buildroot}/opt/multimc
install -m 0644 ../ubuntu/multimc/opt/multimc/icon.svg %{buildroot}/opt/multimc/icon.svg
install -m 0755 ../ubuntu/multimc/opt/multimc/run.sh %{buildroot}/opt/multimc/run.sh
mkdir -p %{buildroot}/%{_datadir}/applications
install -m 0644 ../ubuntu/multimc/usr/share/applications/multimc.desktop %{buildroot}/%{_datadir}/applications/multimc.desktop
mkdir -p %{buildroot}/%{_datadir}/metainfo
install -m 0644 ../ubuntu/multimc/usr/share/metainfo/multimc.metainfo.xml %{buildroot}/%{_datadir}/metainfo/multimc.metainfo.xml
mkdir -p %{buildroot}/%{_mandir}/man1
install -m 0644 ../ubuntu/multimc/usr/share/man/man1/multimc.1 %{buildroot}/%{_mandir}/man1/multimc.1

%files
%dir /opt/multimc
/opt/multimc/icon.svg
/opt/multimc/run.sh
%{_datadir}/applications/multimc.desktop
%{_datadir}/metainfo/multimc.metainfo.xml
%dir /usr/share/man/man1
%{_mandir}/man1/multimc.1.gz

%changelog
* Sun Oct 03 2021 imperatorstorm <30777770+ImperatorStorm@users.noreply.github.com>
- added manpage

* Tue Jun 01 2021 kb1000 <fedora@kb1000.de> - 1.4-2
- Add xrandr to the dependencies

* Tue Dec 08 00:34:35 CET 2020 joshua-stone <joshua.gage.stone@gmail.com>
- Add metainfo.xml for improving package metadata

* Wed Nov 25 22:53:59 CET 2020 kb1000 <fedora@kb1000.de>
- Initial version of the RPM package, based on the Ubuntu package
