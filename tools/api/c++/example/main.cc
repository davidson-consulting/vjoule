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

void evalComputePi () {
    try {
	vjoule::vjoule_api api;

	// The api create a process group containing the pid by default
	// Its name is this
	auto pg = api.get_group ("this");

	// The api can be use to make stamp of consumption for the whole machine
	auto m_begin = api.get_machine_current_consumption ();

	// Using a process group, we can get a stamp of its consumption
	auto p_begin = pg.get_current_consumption (); 

	// perform some computation
	auto pi = computePi (100000000);

	// Get another stamp of consumption
	auto p_end = pg.get_current_consumption ();

	// lose sometime 
	sleep (1);

	// Get a stamp of machine consumption
	auto m_end = api.get_machine_current_consumption ();

	// Compute the difference to get the consumption of the part that interest us
	auto m_diff = m_end - m_begin;
	auto p_diff = p_end - p_begin;

	
	std::cout << "RESULT : " << pi << " " << std::endl;
	std::cout << "THIS : " << p_diff << std::endl;

	// operator '%' can be used to evaluate the percentage of a consumption diff over another
	std::cout << "THIS_PERC : " << (p_diff % m_diff) << std::endl;
	std::cout << "MACHINE : " << m_diff << std::endl;
	
    } catch (vjoule::vjoule_error & err) {
	std::cerr << err.msg << std::endl;
    }    
}


void evalExtern () {
    try {
	vjoule::vjoule_api api;

	// Create a process group for a list of pids
	auto ext_pg = api.create_group ("extern", {12345, 12346});

	// Get a consumption stamp for this process group
	auto e_begin = ext_pg.get_current_consumption ();
	auto m_begin = api.get_machine_current_consumption ();

	// Wait a second to given time for the external processes to consume energy
	sleep (1);

	auto e_end = ext_pg.get_current_consumption ();
	auto m_end = api.get_machine_current_consumption ();

	// display the estimation of the consumption of the external processes during this elapsed period
	std::cout << "EXTERN : " << e_end - e_begin << std::endl;

	// display the real consumption of the machine during this elapsed period of time
	std::cout << "MACHINE : " << m_end - m_begin << std::endl;	
    } catch (vjoule::vjoule_error & err) {
	std::cerr << err.msg << std::endl;
    }
}


int main () {
    evalComputePi ();
    std::cout << "======" << std::endl;
    evalExtern ();    
}
