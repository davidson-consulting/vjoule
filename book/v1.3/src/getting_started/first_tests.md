# First tests

Now that vJoule is installed on your computer, let's try it. Remember that vJoule is available through two interfaces:

- A **command line interface**, useful for simple and quick usage
- A linux **service**, useful if you need to embed vJoule in a program

This section presents just a basic test to make sure vjoule is
properly installed. A more thorough tutorial is presented in the
section [Simple core](./user_guide/simple_core.html).

## vJoule, through the service

vJoule is available through a linux systemd service.

```bash
$ sudo systemctl start vjoule_service
```

You can verify that the service is up either by checking with `systemctl`:

```bash
$ sudo systemctl status vjoule_service
```

Or by checking for the log file : 

```bash
$ cat /etc/vjoule/service.log
```

The default configuration of vJoule only retreive RAPL information. See section
[Simple core](./user_guide/simple_core.html), for more configuration options.

## vJoule, through the command line interface (CLI)

vJoule CLI is conveniently called `vjoule`. You can use it to estimate the energy consumption of a given command, like this: 

```bash
$ # Here, ls is not a subcommand of vjoule
$ # We mesure the energy consummed by the ls command execution
$ vjoule ls

time	21ms856µs
PDU	0.00 J
CPU	0.17 J
RAM	0.03 J
GPU	0.02 J
```

You can also use vjoule on a command that needs parameters. In this case, we will estimate the energy consumption of the `stress` command.

```bash
$ vjoule stress --cpu 2 --timeout 2s
stress: info: [169371] dispatching hogs: 2 cpu, 0 io, 0 vm, 0 hdd
stress: info: [169371] successful run completed in 2s


time	2s53ms193µs
PDU	79.20 J
CPU	40.61 J
RAM	0.00 J
GPU	0.00 J
```

Of course, you can use vjoule to run, say, a Python script.

```bash
$ vjoule python myscript.py

time	1s670ms197µs
PDU	70.40 J
CPU	35.71 J
RAM	2.25 J
GPU	1.78 J
```
