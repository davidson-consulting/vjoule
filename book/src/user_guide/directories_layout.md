# Directories layout
For energy monitoring, we recommand installing the `vjoule_std_sensor` and `vjoule_simple_formula` services (see above for instructions). Once installed, you should find several directories and files of interest on your system:
- `/etc/vjoule/sensor.log`: the log file of the `std_sensor`,
- `/etc/vjoule/simple_formula.log`: the log file of the `simple_formula`,
- `/etc/vjoule/sensor/config.toml`: the configuration file of the `std_sensor`. It can be modified to change the sensor log level and the list of cgroup monitored,
- `/etc/vjoule/simple_formula/config.toml`: the configuration file of the `simple_formula`. It can be modified to change the formula log level.
- `/etc/vjoule/simple_formula/MYCGROUP/`: the directory containing the energy consumption estimation of the cgroup `MYCGROUP`. This where you will find the informations you're looking for. 

## Results
The energy consumption of the cgroup `test` created in the previous example will be stored in self-updating files located in `/etc/vjoule/simple_formula/test.slice/test` (if you've modified the formula configuration to monitor this slice).
vJoule will give you up to four estimations. The main estimation of interest is `package`, that correspond to the energy consummed by the CPU for running your process. Others (not always available) will correspond to the memory, the cpu cores (pp0), the integrated GPU (if any, in pp1) and the dram.

The value stored in the file correspond to the energy consumption, in joule, since vJoule have started monitoring this process.