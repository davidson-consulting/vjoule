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

  void Exporter::export_csv(std::string output_file) {
    Values global = this-> read_for_process(this-> _result_dir);
    Values process = this-> read_for_process(join_path(this-> _result_dir, "vjoule_xp.slice/process"));

    std::ofstream ofs(output_file);
    ofs << "CGroup  ";
    if (this-> _cpu) ofs << "; " << std::setw(11) << "CPU";
    if (this-> _gpu) ofs << "; " <<  std::setw(11) <<"GPU";
    if (this-> _ram) ofs << "; " <<  std::setw(11) <<"RAM";
    ofs << std::endl;

    ofs << "Global  " << this-> value_csv(global) << std::endl;
    ofs << "Process " << this-> value_csv(process) << std::endl;
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

  std::string Exporter::value_csv(Values v){
    std::stringstream ss;
    if (this-> _cpu) {
      ss << "; " << std::setw(10) << v.cpu << "j";
    }
    
    if (this-> _gpu) {
      ss << "; " << std::setw(10) << v.gpu << "j";
    }

    if (this-> _ram) {
      ss << "; " << std::setw(10) << v.ram << "j";
    }
    
    return ss.str();
  }
}
