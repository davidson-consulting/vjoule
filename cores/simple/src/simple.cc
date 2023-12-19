#include "simple.hh"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/mount.h>
#include <cmath>
#include <mntent.h>

using namespace common;

namespace simple {

    Simple::Simple () {}

    bool Simple::configure (const common::utils::config::dict & cfg, common::plugin::Factory & factory) {
		this-> _outputDir = cfg.getOr <std::string> ("output-dir", "/etc/vjoule/results");
		LOG_INFO ("Simple will output in this dir: ", this-> _outputDir);

		if (cfg.getOr <bool> ("mount-tmpfs", true)) {
			this-> mountResultDir ();
		}

        utils::create_directory (this-> _outputDir, true);

        if (!this-> configurePduPlugin (factory)) return false;
		if (!this-> configureGpuPlugins (factory)) return false;
		if (!this-> configureCpuPlugin (factory)) return false;
		if (!this-> configureRamPlugin (factory)) return false;

		return true;
    }

    bool Simple::configureGpuPlugins (common::plugin::Factory & factory) {
		this-> _gpuPlugins = factory.getPlugins ("gpu");
		for (auto & it : this-> _gpuPlugins) {
			auto poll = it-> getFunction<common::plugin::PluginPollFunc_t> ("poll");
			if (poll == nullptr) {
				LOG_ERROR ("Invalid 'gpu' plugin '", it-> getName (), "' has no 'void poll ()' function");
				return false;
			}

			this-> _pollFunctions.emplace (poll);
			auto nbDevice = it-> getFunction <common::plugin::GpuGetNbDevices_t> ("gpu_nb_devices");
			if (nbDevice == nullptr) {
				LOG_ERROR ("Invalid 'gpu' plugin '", it-> getName (), "' has no 'uint32_t gpu_nb_devices ()' function");
				return false;
			}

			std::vector <float> cache;
			cache.resize (nbDevice ());
			this-> _gpuEnergyCache.push_back (std::move (cache));

			auto get = it-> getFunction <common::plugin::GpuGetEnergy_t> ("gpu_get_energy");
			if (get == nullptr) {
				LOG_ERROR ("Invalid 'gpu' plugin '", it-> getName (), "' has no 'void gpu_get_energy (float * energyDevices)' function");
				return false;
			}
			this-> _gpuGet.push_back (get);
		}

		if (this-> _gpuPlugins.size () == 0) {
			LOG_INFO ("No 'gpu' plugin in use");
		}

        this-> _gpuRes = fopen (utils::join_path (this-> _outputDir, "gpu").c_str (), "w");
		return true;
    }

    bool Simple::configurePduPlugin (common::plugin::Factory & factory) {
        auto pdus = factory.getPlugins ("pdu");
        if (pdus.size () > 0) {
            this-> _pduPlugin = pdus [0];
            if (pdus.size () > 1) LOG_WARN ("Core 'simple' can manage only one PDU plugin.");
            auto poll = this-> _pduPlugin-> getFunction <common::plugin::PluginPollFunc_t> ("poll");
            if (poll == nullptr) {
				LOG_ERROR ("Invalid 'pdu' plugin '", this-> _pduPlugin-> getName (), "' has no 'void poll ()' function");
				return false;
			}
			this-> _pollFunctions.emplace (poll);

			auto get = this-> _pduPlugin-> getFunction <common::plugin::PduGetEnergy_t> ("pdu_get_energy");
			if (get == nullptr) {
				LOG_ERROR ("Invalid 'pdu' plugin '", this-> _pduPlugin-> getName (), "' has no 'float pdu_get_energy ()' function");
				return false;
			}
			this-> _pduGet = get;
		} else {
			this-> _pduPlugin = nullptr;
			LOG_INFO ("No 'pdu' plugin in use");
		}

        this-> _pduRes = fopen (utils::join_path (this-> _outputDir, "pdu").c_str (), "w");
		return true;
    }

    bool Simple::configureCpuPlugin (common::plugin::Factory & factory) {
		auto cpus = factory.getPlugins ("cpu");
		if (cpus.size () > 0) {
			this-> _cpuPlugin = cpus[0];
			if (cpus.size () > 1) LOG_WARN ("Core 'simple' can manage only one CPU plugin.");
			auto poll = this-> _cpuPlugin-> getFunction<common::plugin::PluginPollFunc_t> ("poll");
			if (poll == nullptr) {
				LOG_ERROR ("Invalid 'cpu' plugin '", this-> _cpuPlugin-> getName (), "' has no 'void poll ()' function");
				return false;
			}
			this-> _pollFunctions.emplace (poll);

			auto get = this-> _cpuPlugin-> getFunction <common::plugin::CpuGetEnergy_t> ("cpu_get_energy");
			if (get == nullptr) {
				LOG_ERROR ("Invalid 'cpu' plugin '", this-> _cpuPlugin-> getName (), "' has no 'float cpu_get_energy ()' function");
				return false;
			}
			this-> _cpuGet = get;
		} else {
			this-> _cpuPlugin = nullptr;
			LOG_INFO ("No 'cpu' plugin in use");
		}

        this-> _cpuRes = fopen (utils::join_path (this-> _outputDir, "cpu").c_str (), "w");
		return true;
    }

    bool Simple::configureRamPlugin (common::plugin::Factory & factory) {
		auto rams = factory.getPlugins ("ram");
		if (rams.size () > 0) {
			this-> _ramPlugin = rams[0];
			if (rams.size () > 1) LOG_WARN ("Core 'simple' can manage only one RAM plugin.");
			auto poll = this-> _ramPlugin-> getFunction<common::plugin::PluginPollFunc_t> ("poll");
			if (poll == nullptr) {
				LOG_ERROR ("Invalid 'ram' plugin '", this-> _ramPlugin-> getName (), "' has no 'void poll ()' function");
				return false;
			}

			this-> _pollFunctions.emplace (poll);

			auto get = this-> _ramPlugin-> getFunction <common::plugin::CpuGetEnergy_t> ("ram_get_energy");
			if (get == nullptr) {
				LOG_ERROR ("Invalid 'cpu' plugin '", this-> _ramPlugin-> getName (), "' has no 'float ram_get_energy ()' function");
				return false;
			}
			this-> _ramGet = get;
		} else {
			this-> _ramPlugin = nullptr;
			LOG_INFO ("No 'ram' plugin in use");
		}

        this-> _ramRes = fopen (utils::join_path (this-> _outputDir, "ram").c_str (), "w");
		return true;
    }

    void Simple::compute () {
        for (auto & it : this-> _pollFunctions) {
			it ();
		}

		this-> computeCpuEnergy ();
		this-> computeRamEnergy ();
		this-> computeGpuEnergy ();
        this-> computePduEnergy ();

		this-> writeConsumption ();
    }


    void Simple::writeConsumption () {
		if (this-> _gpuRes != nullptr) {
			fseek (this-> _gpuRes, 0, SEEK_SET);
			fprintf (this-> _gpuRes, "%lf", this-> _gpuEnergy);
			fflush (this-> _gpuRes);
        }

        if (this-> _cpuRes != nullptr) {
			fseek (this-> _cpuRes, 0, SEEK_SET);
			fprintf (this-> _cpuRes, "%lf", this-> _cpuEnergy);
			fflush (this-> _cpuRes);
        }

        if (this-> _ramRes != nullptr) {
			fseek (this-> _ramRes, 0, SEEK_SET);
			fprintf (this-> _ramRes, "%lf", this-> _ramEnergy);
			fflush (this-> _ramRes);
        }

        if (this-> _pduRes != nullptr) {
            fseek (this-> _pduRes, 0, SEEK_SET);
            fprintf (this-> _pduRes, "%lf", this-> _pduEnergy);
            fflush (this-> _pduRes);
        }
    }

    void Simple::computeGpuEnergy () {
		for (uint64_t i = 0 ; i < this-> _gpuGet.size () ; i++) {
			this-> _gpuGet[i](this-> _gpuEnergyCache[i].data ());
            for (uint64_t j = 0 ; j < this-> _gpuEnergyCache [i].size () ; j++) {
                this-> _gpuEnergy += this-> _gpuEnergyCache [i][j];
            }
        }
    }

    void Simple::computeRamEnergy () {
		if (this-> _ramGet != nullptr) {
			float ramEnergy = this-> _ramGet ();
			// LLC_MISSES of the system

            this-> _ramEnergy += ramEnergy;
		}
    }

    void Simple::computeCpuEnergy () {
		if (this-> _cpuGet != nullptr) {
			float cpuEnergy = this-> _cpuGet ();
            this-> _cpuEnergy += cpuEnergy;
        }
    }

    void Simple::computePduEnergy () {
        if (this-> _pduGet != nullptr) {
            float pduEnergy = this-> _pduGet ();
            std::cout << this-> _pduEnergy << std::endl;
            this-> _pduEnergy += pduEnergy;
        }
    }

    void Simple::mountResultDir () {
		auto mntType = utils::get_mount_type (this-> _outputDir);
		if (mntType == "tmpfs") {
			umount (this-> _outputDir.c_str ());
		}

		if (!utils::file_exists (this-> _outputDir)) {
			utils::create_directory (this-> _outputDir, true);
		}

		int rc = mount ("tmpfs", this-> _outputDir.c_str (), "tmpfs", 0, "size=512M,uid=0,gid=0,mode=777");
		if (rc != 0) {
			LOG_WARN ("Failed to mount result dir ", this-> _outputDir, " in tmpfs.");
		} else {
			LOG_INFO ("Result dir ", this-> _outputDir, " is mounted in tmpfs");
		}
    }

    void Simple::dispose () {
		if (this-> _cpuRes != nullptr) {
            fclose (this-> _cpuRes);
        }

        if (this-> _gpuRes != nullptr) {
            fclose (this-> _gpuRes);
        }

        if (this-> _ramRes != nullptr) {
            fclose (this-> _ramRes);
        }

        if (this-> _pduRes != nullptr) {
            fclose (this-> _pduRes);
        }

        auto mntType = utils::get_mount_type (this-> _outputDir);
        LOG_INFO (mntType, " ", this-> _outputDir);
		if (mntType == "tmpfs") {
			umount (this-> _outputDir.c_str ());
			utils::remove (this-> _outputDir);
		}

		LOG_INFO ("Disposing simple core.");
    }



}
