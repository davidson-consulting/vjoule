# Cgroups

vJoule monitor the energy consumption of processes through their
cgroup. In a nutshell, a cgroup if a set of linux processes. Cgroups
can be used to limit the resources usage of a set of running
processes. With vJoule, you can estimate the energy consumption of a
set of processes by putting them in a cgroup and asking vJoule to
monitor this cgroup.

For instance, all processes related to vJoule (the vjoule service for
instance) are by default attached to the `vjoule.slice` cgroup.

If you want to monitor the energy consumption of a given process, you
will need to attach this process to a cgroup (you've created) and
configure vJoule to monitor this cgroup. To configure the cgroups
watched by vJoule, the configuration file is stored in
`/etc/vjoule/cgroups` is used. All cgroups watched by vJoule have to
be placed inside slices (cgroup containing cgroups).

In this section we will assume that the configuration of the vJoule
service in `/etc/vjoule/config.toml` was unchanged since installation,
and that the service is started.

```bash
sudo systemctl start vjoule_service.service
```

For instance, let's say you want to measure the energy consumption of the API you've built, there are two steps to perform : 
- Configuring vJoule
- Launching the application

## Configuring vJoule

First we need to inform vJoule that their are cgroups we want to
measure. We will create them later, however the order of attachement
and creation has no importance for vJoule as it will effectively
monitor the cgroups after they are created.  In the file
`/etc/vjoule/cgroups` add the following line :

```
measurements.slice/*
```

This line will configure vJoule to monitor all cgroups in the slice
`measurements.slice`. Bash simple regular expression can be used to be
more specific, for example `measurements.slice/api*` will monitor all
the cgroups in the `measurements.slice` that starts with `api` and
ignore all the others.

There is no need to restart the vjoule service after modifying the
cgroup file, it will reconfigure itself automatically.

## Launching the applications

Now that vJoule is configured to watch the cgroups of our slice, we
can create them and attach processes to them to be seen by vJoule.  To
facilitate the management of cgroups in this section we will use the
commands provided by the packet `cgroup-tools`.

```bash
$ # Create a slice that will contain our cgroups
$ sudo cgcreate -g cpu:measurements.slice

$ # Create a cgroup "api" in a "measurements" slice (a group of cgroup)
$ sudo cgcreate -g cpu:measurements.slice/api

$ # The "measurements" slice could contain other components of your application
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

You don't want to use those linux commands directly ? There is chances you can find librairies to manage cgroups in your favorite programming language.

