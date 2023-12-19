#include <vjoule/vjoule_api.hh>

#include <iostream> // cout
#include <cstdint> // for uint64_t
#include <unistd.h> // for sleep

double computePi (uint64_t prec) {
    double res = 0;
    for (uint64_t i = 0 ; i < prec ; i++) {
        res += (4.0 / prec) / (1.0 + ((i - 0.5) * (1.0 / prec)) * ((i - 0.5) * (1.0 / prec)));
    }
    
    return res;
}

vjoule::consumption_diff_t evalComputePi (uint64_t prec) {
    try {
        vjoule::vjoule_api api;

        // The api can be use to make stamp of consumption for the whole machine
        auto m_begin = api.get_machine_current_consumption ();

        // perform some computation
        auto pi = computePi (prec);



        // Get a stamp of machine consumption
        auto m_end = api.get_machine_current_consumption ();

        // Compute the difference to get the consumption of the part that interest us
        auto m_diff = m_end - m_begin;

	
        std::cout << "RESULT : " << pi << " " << std::endl;
        std::cout << "CONSUMPTION : " << m_diff << std::endl;

        return m_diff;
    } catch (vjoule::vjoule_error & err) {
        std::cerr << err.msg << std::endl;
        throw err;
    }
}

int main () {
    auto fst = evalComputePi (100000000);
    std::cout << "======" << std::endl;
    auto scd = evalComputePi (200000000);

    std::cout << fst % scd << std::endl;
}
