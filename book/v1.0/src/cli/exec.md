# vjoule exec

The *vjoule exec* command can be used to estimate the energy consumption of a given command.

You can use it as follows:

```bash
$ vjoule exec python myscript.py

# Output
|CGroup  |         CPU|         GPU|         RAM|
|--------|------------|------------|------------|
|Global  |    1.16052J|   0.312761J|   0.257263J|
|Process |   0.957794J|          0J|   0.190821J|
```

**Note:** this is the default command in vJoule CLI, you can omit the *exec* subcommand and just use *vJoule*, as follows:

```bash
$ vjoule python myscript.py

# Output
|CGroup  |         CPU|         GPU|         RAM|
|--------|------------|------------|------------|
|Global  |    1.16052J|   0.312761J|   0.257263J|
|Process |   0.957794J|          0J|   0.190821J|
```

If you prefer to store the result output in a csv file, you can use the -o flag, as follows:
```bash
$ vjoule -o output.csv python myscript.py

# Output
$ cat output.csv 
CGroup  ;         CPU;         GPU;         RAM
Global  ;    1.16052J;   0.312761J;   0.257263J;
Process ;   0.957794J;          0J;   0.190821J;
```

vJoule CLI will create a result directory every time it is run. Every results directories are stored under the *__vjoule* directory. The *__vjoule/latest* directory is a symbolic link to the latest run.