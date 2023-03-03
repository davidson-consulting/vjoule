# vJoule API python

The vJoule API for python is accessible from the source repository of vJoule.

## Installation

The installation is made using a local pip install.

```bash
git clone https://github.com/davidson-consulting/vjoule.git
cd vjoule/tools/api/py/src/
pip install .
```

Here is a simple example using the installed API. 

```python
from vjoule_api_py.vjoule_api import *

def computePi(prec):
    res = 0
    for i in range(prec):
		i_prec = 1.0 / prec
        res += (4.0 / prec) / (1.0 + ((i - 0.5) * i_prec) * ((i - 0.5) * i_prec))
    return res


api = VJouleAPI ()
pg = api.getGroup ("this")

m_beg = api.getCurrentMachineConsumption ()
p_beg = pg.getCurrentConsumption ()
	
pi = computePi (10000000)

m_end = api.getCurrentMachineConsumption ()
p_end = pg.getCurrentConsumption ()

m_diff = m_end - m_beg
p_diff = p_end - p_beg

print ("PI : ", pi)
print (p_diff)
print (m_diff)
print (p_diff % m_diff)

api.dispose ()
```


## Permissions

This API uses the vjoule tool `vjoule_cgutils` that can be executed
only by user from the user group `vjoule`. To be part of this group,
the user alice has to type the following command.

```bash
$ usermod -a -G vjoule alice
```

## Usage

The API is base on two important types : `VJouleAPI` and
`ProcessGroup`.  The VJouleAPI type is used to manage process group,
and to retreive the consumption of the whole machine, when process
groups are created by the VJouleAPI and are used to retreive the
consumption of a given cgroup.

The rust API uses the vjoule service, to retreive the consumptions of
the components and cgroups. The API communicates with it to avoid
waiting for service iterations to get the the next consumption, as we
have seen in the user guide.

### Disposing

When the API object is no longer used it has to be disposed, as it
creates cgroups on the system that have to be removed to avoid making
waste material.

```python
api = VJouleAPI ()

# ...

api.dispose ()
```


### Create a new process group

The API can be used to create a new cgroup using a list of pids. 

```python
api = VJouleAPI ()

processGroup = api.createGroup ("my_group", [123, 234, 345])

begin = processGroup.getCurrentConsumption ()
time.sleep (3) # sleeping 3 seconds
end = processGroup.getCurrentConsumption ()

print (end - begin) # display the consumption during the 3 seconds

# Disposing the API, to remove the created groups
api.dispose () 
```

### Retreive an already created cgroup

The API can retreive a cgroup that already exists. As for the cgroup
presented to the vjoule service, the cgroup accessible from the API
must be in a slice.

```python
api = VJouleAPI ()

processGroup = api.getGroup ("slice/group")

begin = processGroup.getCurrentConsumption ()
time.sleep (3) # sleeping 3 seconds
end = processGroup.getCurrentConsumption ()

print (end - begin) # display the consumption during the 3 seconds

# Disposing the API, the cgroup is not removed as it was not created by the API
api.dispose () 
```

By default, a process group named `this` is created by the API when it
is initialized `api.getGroup ("this")`. This process group contains the
pid of the current process, enabling to retreive the estimated
consumption of the running program.
