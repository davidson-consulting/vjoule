// #include <sensor/perf/cpuutils.hh>
// #include <common/utils/_.hh>

// using namespace common;

// namespace sensor::perf {

//     float getMaxFrequency () {
// 	if (utils::file_exists ("/sys/devices/system/cpu/cpufreq/policy0/cpuinfo_max_freq")) {
// 	    auto fd = fopen ("/sys/devices/system/cpu/cpufreq/policy0/cpuinfo_max_freq", "r");
// 	    fscanf (fd, "%d", res);

// 	    fclose (fd);

// 	    return ((float) res) / 1000.0f;
// 	} else {
// 	    utils::Logger::globalInstance ().error ("Failed to read cpu max frequency.");
// 	    return 0;
// 	}
//     }

//     float getMinFrequency () {
// 	if (utils::file_exists ("/sys/devices/system/cpu/cpufreq/policy0/cpuinfo_min_freq")) {
// 	    auto fd = fopen ("/sys/devices/system/cpu/cpufreq/policy0/cpuinfo_max_freq", "r");
// 	    fscanf (fd, "%d", res);

// 	    fclose (fd);

// 	    return ((float) res) / 1000.0f;
// 	} else {
// 	    utils::Logger::globalInstance ().error ("Failed to read cpu min frequency.");
// 	    return 0;
// 	}
//     }

//     float getBaseFrequency () {
// 	if (utils::file_exists ("/sys/devices/system/cpu/cpufreq/policy0/base_frequency")) {
// 	    auto fd = fopen ("/sys/devices/system/cpu/cpufreq/policy0/cpuinfo_max_freq", "r");
// 	    fscanf (fd, "%d", res);

// 	    fclose (fd);

// 	    return ((float) res) / 1000.0f;
// 	} else {
// 	    utils::Logger::globalInstance ().error ("Failed to read cpu base frequency.");
// 	    return 0;
// 	}
//     }

    

// }
