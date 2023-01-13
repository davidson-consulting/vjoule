# First tests

Now that vJoule is installed on your computer, let's try it. Remember that vJoule is available through two interfaces:
- A **command line interface**, useful for simple and quick usage
- A linux **service**, useful if you need to embeed vJoule in a program

## vJoule, through the command line interface (CLI)
vJoule CLI is conveniently called `vJoule`. You can use it to estimate the energy consumption of a given command, like this: 

```
$ vJoule ls
```

You can also use vJoule on a command that needs parameters. In this case, we will estimate the energy consumption of the `stress` command.

```
$ vJoule stress --cpu 2 --timeout 2s
```

Of course, you can use vJoule while executing, let's say, python a script.

```
$ vJoule python myscript.py
```

## vJoule, through the service
As the CLI can be a bit limited for embeeding vJoule in a program, vJoule is available through a linux systemd service.