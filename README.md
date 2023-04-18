# Anon

Anon is a tool to estimate the energy consumption of your softwares.

Anon will estimate the energy consumption of processes running on your computer, based on hardware measurements.

## Installation
### From .deb (recommended)
Anon can be installed from deb files found in the Github releases. The `anon-tools_<VERSION>.deb` package will install a systemd services: `anon_service` (that will estimate the energy consumption
of a list of monitored cgroups and the whole computer)

You can install the .deb file (once downloaded) by running `sudo dpkg -i anon-tools_<VERSION>.deb`.

### From sources
Anons uses CMake as a build tool.

You will need to have installed cmake, g++ and git.

## Documentation
We wrote documentation in a book (using `mdbook`) present in this repository. To build it, you should install `mdbook` first
```
sudo snap install mdbook
```

Then launch the book
```
cd book/v1.0
mdbook serve
```

Or check out the online documentation at https://davidson-consulting.github.io/anon/v1.0/

## Contributing
If you've found any bug, have any issue using anon or would like to propose a new feature, feel free to create a Github issue. 
