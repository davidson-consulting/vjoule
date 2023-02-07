#include <filesystem>
#include <fstream>

#include <common/_.hh>
#include <exporter.hh>

using namespace common::utils;

namespace tools::vjoule {
    
    Exporter::Exporter() {}


    void Exporter::configure (const std::string & result_dir, const std::string & cgroupName, const CommandLine & cmd, std::string time) {
	this-> _result_dir = result_dir;
	this-> _cgroupName = cgroupName;
	this-> _time = time;
	this-> _cpu = cmd.cpu;
	this-> _gpu = cmd.gpu;
	this-> _ram = cmd.ram;
	
	this-> _initGlobal = this-> read_for_process(this-> _result_dir);
	this-> _initProcess =  this-> read_for_process(join_path(this-> _result_dir, this-> _cgroupName));
    }

    void Exporter::export_stdout() {
	Values global = this-> read_for_process(this-> _result_dir) - this-> _initGlobal;
	Values process = this-> read_for_process(join_path(this-> _result_dir, this-> _cgroupName));
	
	std::stringstream ss;
		
	ss << "|CGroup  " << "| ";
	if (this-> _cpu) ss << std::setw(11) << "CPU" << "| ";
	if (this-> _gpu) ss <<  std::setw(11) <<"GPU" << "| ";
	if (this-> _ram) ss <<  std::setw(11) <<"RAM" << "|";
	ss << std::endl;

	ss << "|" << std::string(8, '-');
	if (this-> _cpu) ss << "|" << std::string(12, '-'); 
	if (this-> _gpu) ss << "|" << std::string(12, '-');
	if (this-> _ram) ss << "|" << std::string(12, '-'); 
	ss << "|" << std::endl;
    
	ss << "|Global  " << this-> value_stdout(global) << std::endl;
	ss << "|Process " << this-> value_stdout(process) << std::endl;
    
	std::cout << ss.str() << std::endl;
    }

    void Exporter::export_csv(const std::string & output_file) {
	Values global = this-> read_for_process(this-> _result_dir);
	Values process = this-> read_for_process(join_path(this-> _result_dir, this-> _cgroupName));

	std::ofstream ofs(output_file, std::ofstream::app);
	if (! common::utils::file_exists(output_file)) {
	    ofs << "time; CGroup  ";
	    if (this-> _cpu) ofs << "; " << std::setw(11) << "CPU";
	    if (this-> _gpu) ofs << "; " <<  std::setw(11) <<"GPU";
	    if (this-> _ram) ofs << "; " <<  std::setw(11) <<"RAM";
	    ofs << std::endl;
	}
	
	ofs << this-> _time << "; Global  " << this-> value_csv(global) << std::endl;
	ofs << this-> _time << "; Process " << this-> value_csv(process) << std::endl;
    }

    Values Exporter::read_for_process(const std::string & path) {
	Values v;

	if (this-> _cpu && file_exists(join_path(path, "cpu"))) {
	    std::ifstream fCPU(join_path(path, "cpu"));
	    fCPU >> v.cpu;
	} else v.cpu = 0;

	if (this-> _gpu && file_exists(join_path(path, "gpu"))) {
	    std::ifstream fCPU(join_path(path, "gpu"));
	    fCPU >> v.gpu;
	} else v.gpu = 0;

	if (this-> _ram && file_exists(join_path(path, "ram"))) {
	    std::ifstream fCPU(join_path(path, "ram"));
	    fCPU >> v.ram;
	} else v.ram = 0;

	return v;
    }

    std::string Exporter::value_stdout(Values v){
	char buffer [255];
	std::stringstream ss;
	if (this-> _cpu) {
	    snprintf (buffer, 255, "%10.2fJ", v.cpu);
	    ss << "| " << buffer;
	}
    
	if (this-> _gpu) {
	    snprintf (buffer, 255, "%10.2fJ", v.gpu);
	    ss << "| " << buffer;
	}

	if (this-> _ram) {
	    snprintf (buffer, 255, "%10.2fJ", v.ram);
	    ss << "| " << buffer;
	}
    
	ss << "|";
	return ss.str();
    }

    std::string Exporter::value_csv(Values v){
	std::stringstream ss;
	if (this-> _cpu) {
	    ss << "; " << std::setw(10) << v.cpu << "J";
	}
    
	if (this-> _gpu) {
	    ss << "; " << std::setw(10) << v.gpu << "J";
	}

	if (this-> _ram) {
	    ss << "; " << std::setw(10) << v.ram << "J";
	}
    
	return ss.str();
    }
}
