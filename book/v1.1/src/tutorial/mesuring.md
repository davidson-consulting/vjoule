# Measuring the energy consumption of your program

In this tutorial, we will learn to use vJoule to measure the energy consumption of a simple python script. Please make sure that vJoule is already installed on your computer.

## Your program to evaluate

Let's say you want to measure the energy consumption of the following python script.

```python
import sys

# Compute the value of Pi. 
# A higher value of prec will lead to a higher precision.
def computePi(prec):
    res = 0
    for i in range(prec):
        res += (4.0 / prec) / (1.0 + ((i - 0.5) * (1.0 / prec)) * ((i - 0.5) * (1.0 / prec)))
    return res

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python compute_pi.py precision")
    else:
        print(computePi(int(sys.argv[1])))
```
## Using vJoule CLI

Using vJoule CLI is pretty straightforward, just run python with vJoule:
```
$ vjoule python compute_pi.py 1000  
```

## Using vJoule linux service
vJoule linux service offers a more flexible solution.

### Making sure that vJoule services are up
We will first make sure everything is running with the following commands:
```
$ sudo systemctl status vjoule_service
```

If vjoule service is not running, start it with:
```
$ sudo systemctl start vjoule_service
```

### Creating a cgroup for our process
Remember that vJoule uses cgroups to know which processes to monitor. For a quick introduction to cgroups, refer to the [dedicated section](../user_guide/cgroups.md) in the user guide.

vJoule needs at least a two level cgroup hierarchy. We can organize it as follows:
- A cgroup named `measurements.slice` for our test,
- Inside `measurements.slice`, a cgroup named `compute_pi` in which we will attach our python script.

This can be done with `cgcreate`:
```
$ sudo cgcreate -g cpu:measurements.slice/compute_pi
```

We will configure vJoule to monitor the energy consumption of each cgroups present in `measurements.slice`. To do so, we will edit our service's list of monitored cgroups (present in `/etc/vjoule/cgroups`) by adding `measurements.slice` to the list of cgroups. 

vJoule will now reload itself and create a result directory for our cgroups, in `/etc/vjoule/results/measurements.slice`. Inside, you should have a `compute_pi` directory with a file for every monitored component (CPU, GPU, RAM). 

Now, we can run our python script inside our `compute_pi` cgroup. This can be done with `cgexec`:
```
$ sudo cgexec -g cpu:measurements.slice/compute_pi python compute_pi.py
```

### Getting the results
You should now have a non-zero value in results files. If some are empty, that's because your you did not activated a component or that it was not available.

The value present in the results files are expressed in Joules. It correspond to the amount of energy consumed since vJoule started to monitor the related cgroup.
