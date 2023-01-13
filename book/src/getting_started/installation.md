## Installation
### From .deb (recommended)
Several deb files are currently released. We recommand installing the `vjoule_<VERSION>_no_nfs.deb` package that will install two important systemd services: `vjoule_std_sensor` (that will gather the required hardware infos) and `vjoule_simple_formula` (responsible for the actual estimations of the energy consummed by the monitored processes).

You can install the .deb file (once downloaded) by running `dpkg -i vjoule_<VERSION>_no_nfs.deb`.

### From sources
vJoules uses CMake as a build tool.  