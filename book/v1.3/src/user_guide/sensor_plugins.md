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

The `nvidia` plugin retreives the consumption of nvidia graphics card using
nvml. It can be only be used for the component `gpu`. This plugin does not take
any parameter.

```toml
[gpu]
name = "nvidia"
```

## Yocto plugin

The `yocto` plugin retreives the energy and power consumption of a [YoctoWatt
smart
PDU](https://www.yoctopuce.com/FR/products/capteurs-electriques-usb/yocto-watt).
It can only be used for the component `pdu`. This plugin can take the parameter
`target`. This parameter define the uniq identifier of the YoctoWatt PDU, when
multiple YoctoWatt are connected to the device performing the reading. If only
one smart PDU is connected, this option can simply be ignored.

```toml
[pdu]
name = "yocto"
target = "YWATTMK1-276146"
```

The precision of the yocto plugin is in milliwatt-hours (or 3.6 joules). It has
better precision in immediate power consumption, which is why it provides two
different metrics. RAPL measurement provides micro-joule measurements, so for
really small values it may appear that YoctoWatt has detected 0 consumption,
while RAPL measurement has. It is important to note that 3.6J is really small
and in no way replicable, experiments should be much longer and consume much
more than that to be considered valid (imagine comparing applications with
benchmarks that take only 3.6 microseconds).

## Combining plugins

When there are a nvidia GPU, there can still be an integrated intel GPU. In that
case multiple GPUs consumption are retreivable. To define that in the
configuration file, the following can be written.

```toml
[gpu:0]
name = "rapl"

[gpu:1]
name = "nvidia"
```

Both `simple` and `dumper` cores are able to manage multiple `gpu` plugins, but
only one `cpu`, one `ram` plugin and one `pdu` plugin can be used at the same
time.
