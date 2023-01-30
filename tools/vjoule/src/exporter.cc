#include <filesystem>
#include <fstream>
#include <unordered_map>

#include <common/_.hh>
#include <exporter.hh>

using namespace common::utils;

namespace tools::vjoule {
  Exporter::Exporter(std::string result_dir): _result_dir(result_dir){}

  void Exporter::export_stdout() {
    // read global values 
    Values global = this-> read_for_process(this-> _result_dir);

    // read every process values
    std::vector<std::string> processes = this-> list_processes(); 
    std::unordered_map<std::string, Values> processes_values;
    for (auto p: processes) {
      processes_values[p] = this-> read_for_process(join_path(this-> _result_dir, p));
    }

    std::stringstream ss;
    ss << "VJoule" << std::endl << std::endl;

    ss << "|CGroup " << std::setw(10) << "| CPU | GPU | RAM |" << std::endl;
    ss << "|Global " << global << "|" << std::endl;

    for (auto p: processes) {
      ss << "|" << p << " " << processes_values[p] << "|" << std::endl;
    }

    std::cout << ss.str() << std::endl;
  }

  std::vector<std::string> Exporter::list_processes() {
    int len = this-> _result_dir.length();
    std::vector<std::string> results;
    for (auto& p: std::filesystem::recursive_directory_iterator(this-> _result_dir)) {
      if (p.is_directory()) {
	results.push_back(p.path().string().erase(0, len));
      }
    }

    return results;
  }

  Values Exporter::read_for_process(std::string path) {
    Values v;

    if (file_exists(join_path(path, "cpu"))) {
      std::ifstream fCPU(join_path(path, "cpu"));
      std::stringstream buffer;
      buffer << fCPU.rdbuf();
      v.cpu = stof(buffer.str());
    }

    if (file_exists(join_path(path, "gpu"))) {
      std::ifstream fCPU(join_path(path, "gpu"));
      std::stringstream buffer;
      buffer << fCPU.rdbuf();
      v.gpu = stof(buffer.str());
    }

    if (file_exists(join_path(path, "ram"))) {
      std::ifstream fCPU(join_path(path, "ram"));
      std::stringstream buffer;
      buffer << fCPU.rdbuf();
      v.ram = stof(buffer.str());
    }

    return v;
  }

  std::ostream& operator<< (std::ostream& os, const Values& v) {
    os << "| ";
    if (v.cpu.has_value()) {
      os << "CPU " << v.cpu.value() << "j";
    }
    
    if (v.gpu.has_value()) {
      os << " - GPU " << v.gpu.value() << "j";
    }

    if (v.ram.has_value()) {
      os << " - RAM " << v.ram.value() << "j";
    }
    os << " |";

    return os;
  }
}
