#pragma once

#define __PROJECT__ "SIMPLE"
#define __PLUGIN_VERSION__ "1.3.0"

#include <common/_.hh>
#include <vector>
#include <set>
#include <fstream>

namespace simple {
        class Simple {
        private:

                // directory where results should be written
                std::string _outputDir;

                // The current energy values
                double _pduEnergy;
                double _pduPower;
                float _cpuEnergy;
                float _ramEnergy;
                float _gpuEnergy;

                FILE * _cpuRes;
                FILE * _ramRes;
                FILE * _gpuRes;
                FILE * _pduResEner;
                FILE * _pduResPower;

        private :

                // The list of gpu plugins
                std::vector <common::plugin::Plugin*> _gpuPlugins;

                // The list of get power function of the gpu plugins
                std::vector <common::plugin::GpuGetEnergy_t> _gpuGet;

                // The cache for power consumption reading on gpu
                std::vector <std::vector <float> > _gpuEnergyCache;

        private :

                // The plugin for cpu consumption
                common::plugin::Plugin* _cpuPlugin;

                // The get power function of the cpu plugin
                common::plugin::CpuGetEnergy_t _cpuGet = nullptr;

        private:

                // The plugin for pdu consumption
                common::plugin::Plugin * _pduPlugin;

                // The get energy function of the pdu plugin
                common::plugin::PduGetEnergy_t _pduGetEner = nullptr;

                // The get power function of the pdu plugin
                common::plugin::PduGetEnergy_t _pduGetPower = nullptr;

        private:

                // The plugin for ram consumption
                common::plugin::Plugin* _ramPlugin;

                // The get power function of the ram plugin
                common::plugin::RamGetEnergy_t _ramGet = nullptr;

        private :

                // The list of plugins used by the simple
                // We use a set because we need to poll the plugins only one time, even if they are used for different metrics
                std::set <common::plugin::PluginPollFunc_t> _pollFunctions;

        public :

                /**
                 * Create an empty simple
                 */
                Simple ();

                /**
                 * Configure the simple
                 * @params:
                 *    - cfg: the configuration of the simple
                 */
                bool configure (const common::utils::config::dict & cfg, common::plugin::Factory & factory);

                /**
                 * Poll the energy consumption of the different monitored components, the perf events and output
                 * results in the CSV result files
                 */
                void compute ();

                /**
                 * Dispose the divider and all its handles
                 */
                void dispose ();

        private:

                /**
                 * Configure the gpu plugins
                 */
                bool configureGpuPlugins (common::plugin::Factory & factory);

                /**
                 * Configure the cpu plugin
                 */
                bool configureCpuPlugin (common::plugin::Factory & factory);

                /**
                 * Configure the ram plugin
                 */
                bool configureRamPlugin (common::plugin::Factory & factory);

                /**
                 * Configure the pdu plugin
                 */
                bool configurePduPlugin (common::plugin::Factory & factory);

        private:

                void computeCpuEnergy ();

                void computeRamEnergy ();

                void computeGpuEnergy ();

                void computePduEnergy ();

                void writeConsumption ();

                /**
                 * Mount the result directory in tmpfs
                 */
                void mountResultDir ();

        };

}
