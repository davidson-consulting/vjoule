# First tests

Now that vJoule is installed on your computer, let's try it. Remember that vJoule is available through two interfaces:

- A **command line interface**, useful for simple and quick usage
- A linux **service**, useful if you need to embeed vJoule in a program

## vJoule, through the command line interface (CLI)
vJoule CLI is conveniently called `vjoule`. You can use it to estimate the energy consumption of a given command, like this: 

```bash
$ # Here, ls is not a subcommand of vjoule
$ # We mesure the energy consummed by the ls command execution
$ vjoule ls

###
# RESULT
###
```

You can also use vjoule on a command that needs parameters. In this case, we will estimate the energy consumption of the `stress` command.

```bash
$ vjoule stress --cpu 2 --timeout 2s

###
# RESULT
###
```

Of course, you can use vjoule while executing, let's say, a python script.

```bash
$ vjoule python myscript.py

###
# RESULT
###
```


## vJoule, through the service

As the CLI can be a bit limited for embedding vJoule in a program, vJoule is available through a linux systemd service.

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

By default, vJoule is configured for monitoring the energy consumption of its service. To see how much energy it uses, have a look in the directory `/etc/vjoule/results/vjoule.slice/vjoule_service.service`. You'll see several files where are stored the current energy consummed by vjoule on different components of your computer. By printing the content of the `cpu` file, you will see the energy consummed by the CPU to run the service. Other files may be empty because your hardware do not expose those informations. 

Check out the tutorial section of this book to learn how to use vJoule to monitor the energy consumption of your own processes.
