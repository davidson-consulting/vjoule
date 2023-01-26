# vJoule

vJoule is a tool to estimate the energy consumption of your softwares.

vJoule will estimate the energy consumption of processes running on your computer, based on hardware measurements.

## Installation
### From .deb (recommended)
vJoule can be installed from deb files found in the Github releases. The `vjoule_<VERSION>.deb` package will install three systemd services: `vjoule_sensor` (that will gather the required hardware infos) and `vjoule_simple_formula` (responsible for the actual estimations of the energy consummed by the monitored processes), `vjoule_dumping_formula` (a secondary formula, used for debugging purposes).

You can install the .deb file (once downloaded) by running `dpkg -i vjoule_<VERSION>.deb`.

### From sources
vJoules uses CMake as a build tool.

You will need to have installed cmake, g++ and git.

## Documentation
We wrote documentation in a book (using `mdbook`) present in this repository. To build it, you should install `mdbook` first
```
sudo snap install mdbook
```

Then launch the book
```
mdbook serve book
```

## Contributing
If you've found any bug, have any issue using vjoule or would like to propose a new feature, feel free to create a Github issue. 