# The dumping formula

The dumping formula is a different formula that can be used to dump selected performances counters in a CSV file. You can use the dumping formula in combination with the sensor. 

## Starting the dumping formula
Assuming you already have installed vjoule, you should be able to start the dumping formula as a linux service:
```
$ # if not already done, start the sensor
$ sudo systemctl start vjoule_sensor
$ sudo systemctl start vjoule_dumping_formula
```

## Finding results
The dumping formula will dump every performances counters sent by the sensor in a CSV file. This file can be found in the dumping formula's directory, in `/etc/vjoule/dumping_formula`

## Select the performances counters you want
The sensor fetch two performances counters by default (the number of hardware cpu cycle and the number of last level cache misses). You choose whatever performances counters you want by configuring the sensor (through its configuration file, found in `/etc/vjoule/sensor/config.toml`). 

Please keep in mind that there is some restrictions in the number of performances counters you can query at the same time, especially if you have hyper threading activated on your computer. If you get a lot of zeros in the results, chances are that you tried to select too much performances counters. Try to remove some.