## Installation
### From .deb (recommended)
vJoule can be installed from deb files found in the Github releases. The `vjoule_<VERSION>.deb` package will install three systemd services: `vjoule_sensor` (that will gather the required hardware infos) and `vjoule_simple_formula` (responsible for the actual estimations of the energy consummed by the monitored processes), `vjoule_dumping_formula` (a secondary formula, used for debugging purposes).

You can install the .deb file (once downloaded) by running `dpkg -i vjoule_<VERSION>.deb`.

### From sources
vJoules uses CMake as a build tool.

You will need to have installed cmake, g++ and git.