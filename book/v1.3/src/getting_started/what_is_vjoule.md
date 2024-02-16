## What is vJoule?

vJoule is a tool that can be used to retreive the energy consumption of a linux
operated device. Technically, vJoule is built on a plugin system with two
different kind of plugins :

- `core`, this kind of plugin will define the behavior of vJoule and perform the information gathering from sensor plugins and the effective result computation. In this vJoule version two core plugins are implemented : 
   + `simple` retreives the consumption acquired by sensor plugins and store them in files to be read by external programs. (%CPU, %GPU, etc.).
   + `dumper` retreives the consumption acquired by sensor plugins, and system usage of running processes and dump those metrics in csv files for cold analysis.
- `sensor`, this kind of plugin are used to retreive system information usefull for the core plugin. There are different kind of sensor plugin, in this vJoule version three plugin types are implemented : 
   + `cpu` plugin that retreives the energy consumption of the CPU of the system, (`rapl`)
   + `ram` plugin that retreives the energy consumption of the RAM of the system  (`rapl`)
   + `gpu` plugin that retreives the energy consumption of the GPUs of the system  (`rapl`, `nvidia`)
   + `pdu` plugin that retreives the energy consumption from a Smart PDU (`yocto`)
   
To monitor your consumption of your machine the `simple` core plugin will come
in handy. This documentation will mainly focus on this core plugin.

There are currently have two ways to interact with vJoule :
- As a **command line interface** : useful for easy and quick usage
- As set of APIs in different languages, useful if you want to interact with
  vJoule in a program, and monitor more complex applications.
