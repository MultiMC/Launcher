# What is this?
A simple ubuntu package for MultiMC that wraps the contains a script that downloads and installs real MultiMC on ubuntu based systems.

It contains a `.desktop` file, an icon, and a simple script that does the heavy lifting.

# How to build this?
You need to install the following:

```
sudo dnf install fedora-packager fedpkg fedrepo_req copr-cli
```

Copy the `rpmbuild` folder to `rpmbuild_0.6.7` and then run:
```
rpmbuild -bs rpmbuild_0.6.7/SPECS/multimc.spec
rpmbuild --rebuild rpmbuild_0.6.7/SRPMS/multimc-0.6.7-1.fc31.src.rpm
```

Replace the version with whatever is appropriate.

Taken mostly from the Ubuntu repository