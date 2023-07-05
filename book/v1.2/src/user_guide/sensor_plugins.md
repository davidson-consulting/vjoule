# Sensor plugins

Sensor plugins are plugins used to retreive the energy consumption of
the hardware. They are different type of sensor plugins as there are
different kind of hardware on which we can retreive energy
consumption. In this version of vJoule, there are `cpu`, `ram` and
`gpu` plugins implemented.

Information about the plugins can be retreived by running the command : 

```bash
# vjoule_service --ph plugin_name
# for example

$ vjoule_service --ph rapl
```

## Rapl plugin

The `rapl` plugin retreive the energy consumption of three kind of
hardware, `cpu`, `ram` and `gpu` where here the gpu is the integrated
gpu chip. RAPL is usable on intel cores only.

Depending on the machine, gpu and ram may be unavailable, in that case
during the configuration warnings will be displayed, and ram and gpu
energy consumption retreiving will always return 0.

Warning, there is no perf events on integrated GPU, meaning that the
gpu consumption will be acquired for the whole system but will not be
divided between cgroups when using the `divider` core plugin.

## Nvidia plugin

The `nvidia` plugin retreives the consumption of nvidia graphics card
using nvml. It can be only be used for the component `gpu`.  This
plugin takes only one element of configuration `cgroup-consumption`.

```toml
[gpu]
name = "nvidia"
cgroup-consumption = true
```

If `cgroup-consumption` is true, then the plugin will retreive the
name of the cgroups using the device, and their percentage of usage.
Depending on the graphics card, cgroup usage can be available or
not. Warning messages are displayed if it is not available.  The
plugin is capable of managing multiple devices, if multiple graphics
card are found on the machine.

## Combining plugins

When there are a nvidia GPU, there can still be an integrated intel
GPU. In that case multiple GPUs consumption are retreivable. To define
that in the configuration file, the following can be written.

```toml
[gpu:0]
name = "rapl"

[gpu:1]
name = "nvidia"
```

Both `divider` and `dumper` cores are able to manage multiple `gpu`
plugins, but only one `cpu` and one `ram` plugin can be used at the
same time.
