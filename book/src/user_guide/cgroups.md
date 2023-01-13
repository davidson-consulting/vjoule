# cgroups

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