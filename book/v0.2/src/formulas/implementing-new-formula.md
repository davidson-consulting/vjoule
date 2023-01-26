# Implementing a new formula

To implement a new formula, you can use the `Formula` and `Divider` interfaces. 

## Formula template
The `Formula` template (present in the `common::formula` namespace) provides a bunch a functionnalities required to implement a formula, including:
- Communication with the sensor
- Reconfiguration of the sensor (if specific performances counters are needed)
- Export of the results as files

When implementing a new formula, you can directly use the Formula template parametrized with your own implementation of the divider. This is done as follows:

```
common::formula::Formula<my_formula::Divider>  formula ("my_formula");
formula.run ();
```

## Divider class

The `Divider` interface (present in the `common::formula`) namespace is the main class to implement when building your own formula. When provided as a parameter to a `Formula`, its main function `divideConsumption` will be called when processing packets sent by the sensor. The `Divider` interface provide the following meethods:
-  `configure`: configure the divider from the header
- `divideConsumption`: divide the power consumption from packet information into cgroup consumption
- `getSystemMetrics`: the list of necessary system metrics sent by the sensor for the divider to work
- `getCgroupMetrics`: the list of necessary cgroup metrics sent by the sensor for the divider to work

The last two methods can be use to reconfigure the sensor to get the required performances counters. 

## Results 

The results of your formula are put in the directory according to the
configuration file (`mnt-path`), by default its value is
`/etc/vjoule/my_formula/`.
