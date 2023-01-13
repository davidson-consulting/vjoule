# First tests

Now that vJoule is installed on your computer, let's try it. Remember that vJoule is available through two interfaces:
- A **command line interface**, useful for simple and quick usage
- A linux **service**, useful if you need to embeed vJoule in a program

## vJoule, through the command line interface (CLI)
vJoule CLI is conveniently called `vjoule`. You can use it to estimate the energy consumption of a given command, like this: 

```
$ vjoule ls
```

You can also use vjoule on a command that needs parameters. In this case, we will estimate the energy consumption of the `stress` command.

```
$ vjoule stress --cpu 2 --timeout 2s
```

Of course, you can use vjoule while executing, let's say, python a script.

```
$ vjoule python myscript.py
```

## vJoule, through the service
As the CLI can be a bit limited for embeeding vJoule in a program, vJoule is available through a linux systemd service.

You'll need to start both sensor and formula first: 
```
$ sudo systemctl start vjoule_sensor
$ sudo systemctl start vjoule_simple_formula
```

You can verify that both services are up either by checking with `systemctl`:
```
$ sudo systemctl status vjoule_sensor
$ sudo systemctl status vjoule_simple_formula
```
Or by checking for the logs of both services:
```
$ cat /etc/vjoule/sensor.log
$ cat /etc/vjoule/simple_formula.log
```

By default, vJoule is configured for monitoring the energy consumption of its two services. To see how much energy uses the formula, have a look in the directory `/etc/vjoule/simple_formula/vjoule.slice/vjoule_simple_formula.service`. You'll see several files where are stored the current energy consummed by the formula on different components of your computer. By printing the content of the `package` file, you will see the energy consummed by the CPU to run the formula. Other files may be empty because your hardware do not expose those informations. 

Check out the tutorial section of this book to learn how to use vJoule to monitor the energy consumption of your own processes.