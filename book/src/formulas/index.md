# Formulas

Formulas are the components responsible to infer the energy
consumption of each monitored cgroup from the informations sent by the
sensor. All formulas have the same behavior and work with the same
interfaces.

Two formulas are currently available:
- *simple formula*, that split the overall energy consumption of the host for each monitored cgroup by doing a ratio between a selected performance counter (e.g the CPU time usage) and the RAPL value sent by the sensor,  
- *smartwatts formula*, that uses a linear regression model to infer the energy consumption of monitored cgroup based on performances counters

## Results

When the formula is running, it creates result files for each
monitored cgroups. These files are located in the `mnt-path` defined
in the configuration file. For example for the cgroup `custom/test`
and with `mnt-path = "/etc/vjoule/simple_formula/"`, the
following result files are written :

- `/etc/vjoule/simple_formula/custom/test/package` the cumulated energy consumption of the cgroup from `energy_pkg` (cpu consumption)
- `/etc/vjoule/simple_formula/custom/test/memory` the cumulated energy consumption of the cgroup from `energy_dram` (memory consumption)

When new values are written to the cgroup result files, the formula
updates the file `formula.signal` located in `mnt-path`. This file
only contains a useless value, but is created so we can use the
inotify system to synchronise a puller easily. For example the
following script prints the `package` consumption of the cgroup
`custom/test` when new values are available.

```bash
while inotifywait /etc/vjoule/simple_formula/formula.signal; do 
	cat /etc/vjoule/simple-formula/simple_formula/custom/test/package
done
```

`inotify` is a basic linux system that can be used directly from a
program. Examples of pullers written in bash, C++, and in rust are available
respectively in `puller/bash/`, `puller/c++/` and `puller/rust`.

The following diagrams gives an idea of how formulas interact with the
sensor running on the machine, and how they provides results that can
be used by pullers.

![Formula diagram](images/formula_system.png)
