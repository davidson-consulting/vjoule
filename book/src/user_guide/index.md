# User Guide

## Usage
### Directories layout
For energy monitoring, we recommand installing the `vjoule_std_sensor` and `vjoule_simple_formula` services (see above for instructions). Once installed, you should find several directories and files of interest on your system:
- `/etc/vjoule/sensor.log`: the log file of the `std_sensor`,
- `/etc/vjoule/simple_formula.log`: the log file of the `simple_formula`,
- `/etc/vjoule/sensor/config.toml`: the configuration file of the `std_sensor`. It can be modified to change the sensor log level and the list of cgroup monitored,
- `/etc/vjoule/simple_formula/config.toml`: the configuration file of the `simple_formula`. It can be modified to change the formula log level.
- `/etc/vjoule/simple_formula/MYCGROUP/`: the directory containing the energy consumption estimation of the cgroup `MYCGROUP`. This where you will find the informations you're looking for. 

### How to monitor the energy consumption of a process?
vJoule monitor the energy consumption of processes through their cgroup. In a nutshell, a cgroup if a set of linux processes. Cgroups can be used to limit the resources usage of a set of running processes. With vJoule, you can estimate the energy consumption of a set of processes by putting them in a cgroup and asking vJoule to monitor this cgroup.

For instance, all processes related to vJoule (the sensor and the formula) are by default attached to the `vjoule` cgroup. 

If you want to monitor the energy consumption of a given process, you will need to attach this process to a cgroup (the `vjoule` cgroup or one you've created) and configure the formula to monitor this cgroup (reminder: the configuration file is stored in `/etc/vjoule/simple_formula/config.toml`).

For instance, this can be done in command line like this:
```
# Create a cgroup "test" in a "test" slice (a group of cgroup)
sudo cgcreate -g cpu:test.slice/test

# Run a command that will be attached to the test cgroup
sudo cgexec -g cpu:test.slice/test mycommand

# OR

# Attach an existing process, by its pid, to the test cgroup
sudo cgclassify -g cpu:test.slice/test mypid
```

As you've seen you can create a hierarchy of cgroups that will be monitored by vJoule (here you need to add `test.slice` to the monitored cgroups in the formula configuration). 

### Understanding the results
The energy consumption of the cgroup `test` created in the previous example will be stored in self-updating files located in `/etc/vjoule/simple_formula/test.slice/test` (if you've modified the formula configuration to monitor this slice).
vJoule will give you up to four estimations. The main estimation of interest is `package`, that correspond to the energy consummed by the CPU for running your process. Others (not always available) will correspond to the memory, the cpu cores (pp0), the integrated GPU (if any, in pp1) and the dram.

The value stored in the file correspond to the energy consumption, in joule, since vJoule have started monitoring this process.

### Fetching the results
We have implemented several `pullers` programs that will fetch the results. Feel free to use them (see the dedicated section in the documentation). 