## What is vJoule?
vJoule is a tool that can be used to estimate the energy consumption a
set of running processes. Technically, vJoule is built on a plugin
system with two different kind of plugins :

- `core`, this kind of plugin will define the behavior of vJoule and perform the information gathering from sensor plugins and the effective result computation. In this vJoule version two core plugins are implemented : 
   + `divider` splits the consumption acquired by sensor plugins, and divides them between the running processes based on their relative system usage (%CPU, %GPU, etc.).
   + `dumper` retreives the consumption acquired by sensor plugins, and system usage of running processes and dump those metrics in csv files for cold analysis.
- `sensor`, this kind of plugin are used to retreive system information usefull for the core plugin. There are different kind of sensor plugin, in this vJoule version three plugin types are implemented : 
   + `cpu` plugin that retreives the energy consumption of the CPU of the system, (`rapl`)
   + `ram` plugin that retreives the energy consumption of the RAM of the system  (`rapl`)
   + `gpu` plugin that retreives the energy consumption of the GPUs of the system  (`rapl`, `nvidia`)


<!-- ![vJoule main concept]() -->

You currently have two ways to interact with vJoule :
- As a **command line interface** : useful for easy and quick usage
- As a **linux service** : useful if you want to interact with vJoule in a program 

### Current limitations
vJoule cannot be used inside a virtual machine as it needs direct access to the hardware.
