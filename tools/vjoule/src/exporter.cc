#include <filesystem>
#include <fstream>

#include <common/_.hh>
#include <exporter.hh>

using namespace common::utils;

namespace tools::vjoule {
  Exporter::Exporter(std::string result_dir, bool cpu, bool gpu, bool ram): 
    _result_dir(result_dir),
    _cpu(cpu),
    _gpu(gpu),
    _ram(ram)
  {}

  void Exporter::export_stdout() {
    Values global = this-> read_for_process(this-> _result_dir);
    Values process = this-> read_for_process(join_path(this-> _result_dir, "vjoule_xp.slice/process"));

    std::stringstream ss;
    ss << "VJoule" << std::endl << std::endl;

    ss << "|CGroup  " << "| ";
    if (this-> _cpu) ss << std::setw(11) << "CPU" << "| ";
    if (this-> _gpu) ss <<  std::setw(11) <<"GPU" << "| ";
    if (this-> _ram) ss <<  std::setw(11) <<"RAM" << "|";
    ss << std::endl;

    ss << "|" << std::string(8, '-') << "|" << std::string(12, '-') << "|" << std::string(12, '-') << "|" << std::string(12, '-') << "|" << std::endl;
    
    ss << "|Global  " << this-> value_stdout(global) << std::endl;
    ss << "|Process " << this-> value_stdout(process) << std::endl;
    
    std::cout << ss.str() << std::endl;
  }

  Values Exporter::read_for_process(std::string path) {
    Values v;

    if (this-> _cpu && file_exists(join_path(path, "cpu"))) {
      std::ifstream fCPU(join_path(path, "cpu"));
      std::stringstream buffer;
      buffer << fCPU.rdbuf();
      v.cpu = stof(buffer.str());
    }

    if (this-> _gpu && file_exists(join_path(path, "gpu"))) {
      std::ifstream fCPU(join_path(path, "gpu"));
      std::stringstream buffer;
      buffer << fCPU.rdbuf();
      v.gpu = stof(buffer.str());
    }

    if (this-> _ram && file_exists(join_path(path, "ram"))) {
      std::ifstream fCPU(join_path(path, "ram"));
      std::stringstream buffer;
      buffer << fCPU.rdbuf();
      v.ram = stof(buffer.str());
    }

    return v;
  }

  std::string Exporter::value_stdout(Values v){
    std::stringstream ss;
    if (this-> _cpu) {
      ss << "| " << std::setw(10) << v.cpu << "j";
    }
    
    if (this-> _gpu) {
      ss << "| " << std::setw(10) << v.gpu << "j";
    }

    if (this-> _ram) {
      ss << "| " << std::setw(10) << v.ram << "j";
    }
    
    ss << "|";
    return ss.str();
  }
}
