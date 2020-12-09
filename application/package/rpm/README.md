# What is this?
A simple RPM package for MultiMC that contains a script that downloads and installs real MultiMC on Red Hat based systems.

It contains a `.desktop` file, an icon, and a simple script that does the heavy lifting.

# How to build this?
You need the `fedora-packager` and `fedora-review` packages. Switch into this directory, then run:

```bash
fedpkg --release f33 local
```

Replace the release with whatever is appropriate for your distribution. ()
An RPM is available to be installed in the x86_64 folder (or i*86 for 32bit machines.)

You can install this via your package manager (dnf/yum)
eg.
```bash
sudo dnf install $(uname -i)/MultiMC5-develop-1.5.fc33.$(uname -i).rpm
```
