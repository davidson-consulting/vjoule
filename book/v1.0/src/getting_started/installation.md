## Installation
### From .deb (recommended)

vJoule can be installed from deb files found in the Github
releases. The `vjoule_<VERSION>.deb` package will install a systemd
service: `vjoule_service` and a client program simply named `vjoule`.

You can install the .deb file (once downloaded) by running `dpkg -i vjoule_<VERSION>.deb`.

### From sources

vJoules uses CMake as a build tool.

You will need to have installed cmake, g++, git and nvidia-ml.
