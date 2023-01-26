# cgroups

vJoule monitor the energy consumption of processes through their cgroup. In a nutshell, a cgroup if a set of linux processes. Cgroups can be used to limit the resources usage of a set of running processes. With vJoule, you can estimate the energy consumption of a set of processes by putting them in a cgroup and asking vJoule to monitor this cgroup.

For instance, all processes related to vJoule (the sensor and the formula) are by default attached to the `vjoule` cgroup. 

If you want to monitor the energy consumption of a given process, you will need to attach this process to a cgroup (the `vjoule` cgroup or one you've created) and configure the formula to monitor this cgroup (reminder: the configuration file is stored in `/etc/vjoule/simple_formula/config.toml`).

For instance, let's say you want to measure the energy consumption of the API you've built. This can be done in command line like this:
```
$ # Create a cgroup "api" in a "measurements" slice (a group of cgroup)
$ sudo cgcreate -g cpu:measurements.slice/api

$ # The "measurements" slice could contains other components of you application
$ # Like the database for instance
$ # To do so :
$ sudo cgcreate -g cpu:measurements.slice/database

$ # Run a command that will be attached to the api cgroup
$ sudo cgexec -g cpu:measurements.slice/api mycommand

$ # OR

$ # Attach an existing process, by its pid, to the api cgroup
$ sudo cgclassify -g cpu:measurements.slice/api mypid
$ # TIP : For a process with multiple PIDs, you can use pidof like:
$ sudo cgclassify -g cpu:measurements.slice/api `pidof apache2`
```
Should you use `cgexec` or `cgclassify`? It depends on your context. If you want to monitor a running process like a webserver, use `cgclassify` with the webserver's pid. If you want to run a command and see how much energy it consummed during its execution, use `cgexec`. 

You don't want to use those linux commands directly? There is chances you can find librairies to manage cgroups in your favorite programming language.

As you've seen you can create a hierarchy of cgroups that will be monitored by vJoule (here you need to add `measurements.slice` to the monitored cgroups in the sensor configuration). 