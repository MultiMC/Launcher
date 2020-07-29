# What is this?
A simple ubuntu package for MultiMC that wraps the contains a script that downloads and installs real MultiMC on ubuntu based systems.

It contains a `.desktop` file, an icon, and a simple script that does the heavy lifting.

# How to build this?
You need devscripts. First run 'dch -v <version>' to create a new changelog
entry.  Replace '<version'> with the appropriate version.  Next, run 'debuild'
to create the deb.
