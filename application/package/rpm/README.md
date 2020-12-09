# What is this?
A simple RPM package for MultiMC that contains a script that downloads and installs real MultiMC on Red Hat based systems.

It contains a `.desktop` file, an icon, and a simple script that does the heavy lifting.

# How to build this?
You'll need the `rpm-build` package. Switch into this directory, then run:
They can be installed on fedora with the following:
```bash
sudo dnf builddep MultiMC5.spec
```
When installed; switch into this directory, then run:

```
rpmbuild --build-in-place -bb MultiMC5.spec
```

Replace the release with whatever is appropriate for your distribution.
An RPM is available to be installed in the x86_64 folder (or i*86 for 32bit machines.)

You can install this via your package manager with:(dnf/yum)

eg.
```bash
sudo dnf install $(uname -i)/MultiMC5-0-1.5.fc33.$(uname -i).rpm
```
