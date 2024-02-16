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
m_beg = api.getCurrentMachineConsumption ()
	
pi = computePi (10000000)

m_end = api.getCurrentMachineConsumption ()

m_diff = m_end - m_beg

print ("PI : ", pi)
print (m_diff)
```

```shell
$ python main.py
PI :  3.1415928535904256
diff (time: 2.00s, pdu: 83.10J, cpu: 41.89J, ram1.56J, gpu: 0.30J)
```

## Usage

The API is base on the type `VJouleAPI`. This type is used to retreive
the consumption of the machine from different components (enabled by the
configuration of the vjoule_service).

The Python API uses the vjoule service to retrieve the consumption of the
components. The service must be running, using the `simple` core plugin. The
`getCurrentMachineConsumption` function triggers a consumption reading by the
service, and retrieves the values of each enabled component (disabled components
are set to 0). So there is no need to wait for the next iteration of the service
to read a value. In fact, the service can be configured with a frequency of 0
(i.e., no iteration at all).
