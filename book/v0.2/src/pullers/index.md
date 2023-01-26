# Pullers


Pullers are programs acquiring the result computed by a formula in
order to display it, or use it for computation. There are example of
pullers in the directory `puller/`.

## Bash `puller/bash`

This is the simplest puller that can be designed. It simple waits for
the `simple_formula` signals, and print the consumption of the cgroup
`custom/test`. (Cf. [results](../simple-formula/index.html#results))


## C++ `puller/c++`

This c++ version of the puller is an example that read `100` formula
reports from the `simple_formula`. It prints the consumption of all
the monitored cgroup.

## Rust `puller/rust`

This rust version is an infinite loop that prints the consumption of
all the monitored cgroup from the `simple_formula`.

## Python `puller/py`

Python puller are more advanced puller, that can be used to display
data in graphs or grafana.

### to_plot

To plot pulls the results of a formula (given as parameter) and plot
the result into a figure. The program runs indefinitely until `Ctrl-C`
is pressed, then it save the results of the consumption into a line
graph.

```bash
$ python3 to_plot.py simple
09:51:34/0
Reading /etc/vjoule/simple_formula/custom/test
09:51:35/1
Reading /etc/vjoule/simple_formula/custom/test
09:51:36/2
Reading /etc/vjoule/simple_formula/custom/test
09:51:37/3
Reading /etc/vjoule/simple_formula/custom/test
09:51:38/4
Reading /etc/vjoule/simple_formula/custom/test
09:51:39/5
^C
Figure exported to out.jpg
```

formula names are used to create the path of result pulling, where
signal is located in
`/etc/vjoule/${formula}_formula/formula.signal` and results in
`/etc/vjoule/${formula}_formula/`.


### to_influxdb

This puller saves the result into an influxdb instance, that can then
be used to monitor the results using grafana. 

```bash 
$ python3 to_influxdb.py simple
09:58:19.607731
Reading : /etc/vjoule/simple_formula/vjoule.slice/vjoule_sensor.service
Reading : /etc/vjoule/simple_formula/vjoule.slice/vjoule_simple_formula.service
09:58:20.667710
Reading : /etc/vjoule/simple_formula/vjoule.slice/vjoule_sensor.service
Reading : /etc/vjoule/simple_formula/vjoule.slice/vjoule_simple_formula.service
09:58:21.667741
Reading : /etc/vjoule/simple_formula/vjoule.slice/vjoule_sensor.service
Reading : /etc/vjoule/simple_formula/vjoule.slice/vjoule_simple_formula.service
09:58:22.675729
Reading : /etc/vjoule/simple_formula/vjoule.slice/vjoule_sensor.service
Reading : /etc/vjoule/simple_formula/vjoule.slice/vjoule_simple_formula.service
09:58:23.679734
Reading : /etc/vjoule/simple_formula/vjoule.slice/vjoule_sensor.service
Reading : /etc/vjoule/simple_formula/vjoule.slice/vjoule_simple_formula.service
09:58:24.679768
Reading : /etc/vjoule/simple_formula/vjoule.slice/vjoule_sensor.service
Reading : /etc/vjoule/simple_formula/vjoule.slice/vjoule_simple_formula.service
09:58:25.667689
Reading : /etc/vjoule/simple_formula/vjoule.slice/vjoule_sensor.service
Reading : /etc/vjoule/simple_formula/vjoule.slice/vjoule_simple_formula.service
09:58:26.675723
Reading : /etc/vjoule/simple_formula/vjoule.slice/vjoule_sensor.service
Reading : /etc/vjoule/simple_formula/vjoule.slice/vjoule_simple_formula.service
```

Results are stored in `localhost:8086` using user `root` in the
database `vjoule-${formula}`.
