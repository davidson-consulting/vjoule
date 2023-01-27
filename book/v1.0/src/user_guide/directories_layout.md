# Files of interests

Once vJoule is installed, you should find a directory located in
`/etc/vjoule` that contains configurations, logs and results. For
example, the service configuration is located in
`/etc/vjoule/config.toml`.

## Configuration files and logs

- `/etc/vjoule/config.toml`: the configuration file of the `vjoule_service`. It can be modified to change the log level and some other options that depends on the core plugin used.
- `/etc/vjoule/service.log`: the log file of the `vjoule_service`,

## Energy estimation results

Assuming you are using the `divider` core plugin (specified in the
configuration file of the vjoule_service), the directory
`/etc/vjoule/results/MYSLICE/MYCGROUP` contains the energy consumption
estimation of the cgroup `MYSLICE/MYCGROUP`. This is where you will
find the informations you're looking for.

By default, vJoule is configured to monitor its own service in a
cgroup named `vjoule.slice`. The energy consumption of the sensor will
then be stored in self-updating files located in
`/etc/vjoule/results/vjoule.slice/vjoule_service.service`.

vJoule will give you up to five results files:
- ram - The energy consumption of the RAM
- cpu - The energy consumption of the CPU
- gpu - The energy consumption of the GPUs (sum if there are multiple gpus)

The value stored in the file correspond to the energy consumption, in **joule**, since vJoule have started monitoring these processes.

The `divider` core plugin also dumps the energy consumption of the
whole system at the root of the result directory
`/etc/vjoule/results`.  There can be found the three files `cpu`,
`ram` and `gpu` that contains the energy consumption in **joule** of
the whole system since the start of the vJoule service (as acquired by
the sensor plugins).

