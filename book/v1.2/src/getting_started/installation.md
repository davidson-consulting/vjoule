## Installation

### From deb package (recommended)

vJoule can be installed from deb files found in the Github
releases. The `vjoule-tools_<VERSION>.deb` package will install a
systemd service: `vjoule_service` and a client program simply named
`vjoule`.

You can install the `.deb` file (once downloaded) by running `dpkg -i
vjoule-tools_<VERSION>.deb`.


### From sources

vJoules uses CMake as a build tool.

You will need to have installed cmake, g++, git and nvidia-ml.

```bash
git clone https://github.com/davidson-consulting/vjoule.git

cd vjoule
mkdir .build
cd .build 
cmake ..
make -j8

sudo make install
```
