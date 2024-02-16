# vJoule API in C++

The vJoule API for C++ is installed along with the vJoule service. No other
installations are required.

Here is a simple example using the installed API.

```c++
#include <iostream>
#include <cstdint>
#include <vjoule/vjoule_api.hh>

double computePi (uint64_t prec) {
    double res = 0;
    for (uint64_t i = 0 ; i < prec ; i++) {
        res += (4.0 / prec) / (1.0 + ((i - 0.5) * (1.0 / prec)) * ((i - 0.5) * (1.0 / prec)));
    }
    
    return res;
}

int main () {
    vjoule::vjoule_api api;
    
    auto m_begin = api.get_machine_current_consumption ();
    auto pi = computePi (100000000);
    auto m_end = api.get_machine_current_consumption ();
        
    auto m_diff = m_end - m_begin;

    std::cout << "RESULT : " << pi << " " << std::endl;
    std::cout << "CONSUMPTION : " << m_diff << std::endl;
    
    return 0;
}
```

The compilation needs to the link the library `libvjoule_cpp`.
```shell
$ g++ main.cc -lvjoule_cpp
$ ./a.out
RESULT : 3.14159 
CONSUMPTION : diff (time: 0.51s, pdu: 19.30J, cpu: 9.86J, ram: 0.31J, gpu: 0.00J)
```

## Usage

The API is base on the type `vjoule::vjoule_api`. This type is used to retreive
the consumption of the machine from different components (enabled by the
configuration of the vjoule_service).

The C++ API uses the vjoule service to retrieve the consumption of the
components. The service must be running, using the `simple` core plugin. The
`getCurrentMachineConsumption` function triggers a consumption reading by the
service, and retrieves the values of each enabled component (disabled components
are set to 0). So there is no need to wait for the next iteration of the service
to read a value. In fact, the service can be configured with a frequency of 0
(i.e., no iteration at all).
