# class sensor::cgroup::CgroupLister

Relatively simple class that traverse the cgroup tree, to retreive the
names and path of the cgroups that must be watched by the sensor.

The class takes a `common::utils::config::dict` as parameters. This
configuration is the `[cgroups]` parts of the configuration file of
the sensor. `CgroupLister::run` returns the list of cgroup to watch.
