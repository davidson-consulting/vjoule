# Files of interests
Once vJoule is installed, you should find a directory for each vJoule services. Those directories (located in `/etc/vjoule`) contains configurations, logs and results. For example, the sensor configuration is located in `/etc/vjoule/sensor` and the simple formula configuration is located in `/etc/vjoule/simple_formula`.

## Configuration files and logs
### Log files
- `/etc/vjoule/sensor.log`: the log file of the `std_sensor`,
- `/etc/vjoule/simple_formula.log`: the log file of the `simple_formula`

### Configuration files
- `/etc/vjoule/sensor/config.toml`: the configuration file of the `std_sensor`. It can be modified to change the sensor log level and the list of cgroup monitored,
- `/etc/vjoule/simple_formula/config.toml`: the configuration file of the `simple_formula`. It can be modified to change the formula log level.

## Energy estimation results
`/etc/vjoule/simple_formula/MYCGROUP/` contains the energy consumption estimation of the cgroup `MYCGROUP`. This where you will find the informations you're looking for. 

By default, vJoule is configured to monitor its own services in a cgroup named `vjoule.slice`. The energy consumption of the sensor will then be stored in self-updating files located in `/etc/vjoule/simple_formula/vjoule.slice/vjoule_sensor.service`.

vJoule will give you up to five results files:
- memory - The energy consumption of the RAM
- package - The energy consumption of the CPU
- pp0 - The energy consumption of the CPU cores only
- pp1 - The energy consumption of the CPU uncores and last level cache
- psys - The energy consumption of the CPU socket

The main estimation of interest is `package`, that correspond to the energy consummed by the CPU for running your process. Other values are not always available, depending on what your hardware exposes.

The value stored in the file correspond to the energy consumption, in joule, since vJoule have started monitoring this process.