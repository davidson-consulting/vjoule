# vjoule exec

The *vjoule exec* command can be used to estimate the energy consumption of a given command.

You can use it as follows:

```bash
$ vjoule exec python myscript.py

time	1s670ms197µs
PDU	0.00 J
CPU	35.71 J
RAM	2.25 J
GPU	1.78 J
```

**Note:** this is the default command in vJoule CLI, you can omit the *exec* subcommand and just use *vJoule*, as follows:

```bash
$ vjoule python myscript.py

time	1s670ms197µs
PDU	0.00 J
CPU	35.71 J
RAM	2.25 J
GPU	1.78 J
```
