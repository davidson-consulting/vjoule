#include <common/perf/rapl_utils.hh>
#include <common/utils/log.hh>
#include <common/utils/files.hh>
#include <common/concurrency/proc.hh>
#include <common/concurrency/timer.hh>
#include <common/net/ports.hh>

#include <rapl.hh>
#include <filesystem>
#include <netdb.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sysinfo.h>

using namespace common;
using namespace common::utils;
using namespace common::perf;
namespace fs = std::filesystem;

namespace tools::vjoule
{

	RaplReader::RaplReader()
	{
		std::cout << "SYSTEM INFOS" << std::endl;
		if (this->openMsrFiles())
		{
			this->openCpuTempFile();
			this->openFrequencyFiles();
		}
	}

	/**
	 * ================================================================================
	 * ================================================================================
	 * =========================        BARE METAL CFG         ========================
	 * ================================================================================
	 * ================================================================================
	 */

	bool RaplReader::openMsrFiles()
	{
		this->_bare.cpuModel = detect_cpu();
		this->_bare.packageMap = detect_packages(this->_bare.totalPackages, this->_bare.totalCores);
		this->_bare.raplAvail = detect_avail(0, this->_bare.cpuModel);

		try
		{
			for (auto &it : this->_bare.packageMap)
			{
				this->_bare.fds.push_back(open_msr(it));

				auto packageUnit = read_package_unit(this->_bare.fds.back(), this->_bare.raplAvail);
				this->_bare.packageUnits.push_back(packageUnit);

				std::cout << "Power units[" << it << "] = " << packageUnit.powerUnits << "W" << std::endl;
				std::cout << "CPU energy units[" << it << "] = " << packageUnit.cpuEnergyUnits << "J" << std::endl;
				std::cout << "DRAM energy units[" << it << "] = " << packageUnit.dramEnergyUnits << "J" << std::endl;
				std::cout << "Time units[" << it << "] = " << packageUnit.timeUnits << "s" << std::endl;
				this->_bare.lastValues.push_back(read_package_values(this->_bare.fds.back(), this->_bare.raplAvail, packageUnit));
			}

			return true;
		}
		catch (...)
		{
			this->dispose(); // disposing because some elements where opened even if they failed to provide data

			std::cout << "RaplReader : failed to configure in bare metal mode." << std::endl;
			return false;
		}
	}

	void RaplReader::openCpuTempFile()
	{
		for (int i = 0;; i++)
		{
			std::stringstream filename;
			filename << "/sys/class/thermal/thermal_zone" << i << "/type";
			std::ifstream type(filename.str().c_str());
			if (type.is_open())
			{
				std::string value;
				type >> value;
				if (value == "x86_pkg_temp")
				{
					std::stringstream tempname;
					tempname << "/sys/class/thermal/thermal_zone" << i << "/temp";
					this->_bare.tempFile = tempname.str();
					return;
				}
			}
			else
				break;
		}

		std::cout << "Failed to find cpu temperature file" << std::endl;
	}

	void RaplReader::openFrequencyFiles()
	{
		for (int i = 0;; i++)
		{
			std::stringstream filename;
			filename << "/sys/devices/system/cpu/cpu" << i << "/cpufreq/scaling_cur_freq";
			std::ifstream f(filename.str().c_str());
			if (f.is_open())
			{
				this->_bare.freqFiles.push_back(filename.str());
			}
			else
				break;
		}
		if (this->_bare.freqFiles.size() < get_nprocs()) {
			std::cout << "Found " << this->_bare.freqFiles.size() << " frequency files for " << get_nprocs() << " CPU" << std::endl;
		}
	}

	/**
	 * ================================================================================
	 * ================================================================================
	 * =========================        RESULT POLLING         ========================
	 * ================================================================================
	 * ================================================================================
	 */

	void RaplReader::poll(common::net::Packet &packet)
	{
		packet.energy_pp0 = 0;
		packet.energy_pp1 = 0;
		packet.energy_pkg = 0;
		packet.energy_dram = 0;
		packet.energy_psys = 0;

		this->pollBareMetal(packet);
	}

	void RaplReader::pollBareMetal(common::net::Packet &packet)
	{
		for (size_t i = 0; i < this->_bare.fds.size(); i++)
		{
			auto values = read_package_values(this->_bare.fds[i], this->_bare.raplAvail, this->_bare.packageUnits[i]);
			packet.energy_pp0 += (values.pp0 - this->_bare.lastValues[i].pp0);
			packet.energy_pp1 += (values.pp1 - this->_bare.lastValues[i].pp1);
			packet.energy_dram += (values.dram - this->_bare.lastValues[i].dram);
			packet.energy_pkg += (values.package - this->_bare.lastValues[i].package);
			packet.energy_psys += (values.psys - this->_bare.lastValues[i].psys);

			this->_bare.lastValues[i] = values;
		}

		if (this->_bare.tempFile != "")
		{
			std::ifstream f(this->_bare.tempFile);
			f >> packet.core_temp;
			f.close();
		}

		packet.cpuFreq.resize(this->_bare.freqFiles.size());
		for (int i = 0; i < this->_bare.freqFiles.size(); i++)
		{
			std::ifstream f(this->_bare.freqFiles[i]);
			f >> packet.cpuFreq[i];
			f.close();
		}
	}

	/**
	 * ================================================================================
	 * ================================================================================
	 * ===========================        DISPOSING         ===========================
	 * ================================================================================
	 * ================================================================================
	 */

	void RaplReader::dispose()
	{
		for (auto &it : this->_bare.fds)
		{
			close(it);
		}

		this->_bare.fds.clear();
		this->_bare.packageUnits.clear();
		this->_bare.totalCores = 0;
		this->_bare.totalPackages = 0;
		this->_bare.cpuModel = -1;
		this->_bare.raplAvail = {};
		this->_bare.packageMap.clear();
		this->_bare.tempFile = "";
		this->_bare.freqFiles.clear();
	}

	RaplReader::~RaplReader()
	{
		this->dispose();
	}
}
