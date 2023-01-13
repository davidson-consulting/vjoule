## What is vJoule?
vJoule is a tool that can be used to estimate the energy consumption a set of running processes. Technically, vJoule is built from two components:
- The *sensor*, responsible to collect different metrics (current energy consumption of the host and performances counters)
- The *formula*, responsible to estimate the energy consumption of each monitored processes, based on the informations gathered by the sensor. 

It a nutshell, vJoules split the energy consumption of the host (*via* the sensor) among the monitored processes (*via* the formula).

![vJoule main concept](images/vJoule_principles.svg)

You currently have two ways to interact with vJoule :
- As a **command line interface** : useful for easy and quick usage
- As a **linux service** : useful if you want to interact with vJoule in a program 

### Current limitations
vJoule cannot be used inside a virtual machine as it needs direct access to the hardware.