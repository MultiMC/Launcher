Name:           multimc
Version:        0.6.7
Release:        1%{?dist}
Summary:        Free, open source launcher and instance manager for Minecraft

License:        ASL 2.0
URL:            https://multimc.org/
Source0:        https://github.com/MultiMC/MultiMC5/releases/download/%{version}/mmc-stable-lin64.tar.gz

Requires:       bash
Requires:       java-1.8.0-openjdk
Requires:       java-1.8.0-openjdk-devel
Requires:       qt5

%description
Free, open source launcher and instance manager for Minecraft

%prep
%autosetup


%install
mkdir -p %{buildroot}/opt/multimc
mkdir -p %{buildroot}/usr/share/applications
install -m 0755 usr/share/applications/multimc.desktop %{buildroot}/usr/share/applications/multimc.desktop
install -m 0755 opt/multimc/run.sh %{buildroot}/opt/multimc/run.sh
install -m 0755 opt/multimc/icon.svg %{buildroot}/opt/multimc/icon.svg

%files
%license LICENSE
/usr/share/applications/multimc.desktop
/opt/multimc/run.sh
/opt/multimc/icon.svg


%changelog
* Sun Nov  3 2019 Sambhav Saggi <smsaggi@gmail.com>
- Initial MultiMC Fedora Package.
- Still must resolve dependencies
