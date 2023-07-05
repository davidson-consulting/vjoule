# Trouble-shooting

## A. vJoule does not seem to start...

The log file `/etc/vjoule/service.log` is very useful to determine the reason of the error.
 
Here are some common errors with some explanations : 

 - `[Error][DIVIDER] Cgroup v2 not mounted, only cgroup v2 is supported.`
	
	In this version of vJoule only cgroup v2 are supported. Maybe you have cgroup v1 installed on your system. [https://rootlesscontaine.rs/getting-started/common/cgroup2/](https://rootlesscontaine.rs/getting-started/common/cgroup2/).
	
	The same error can appear on different component, dumper core and nvidia sensor plugins need cgroup v2 to work.
	
 - `[Error][RAPL] RaplReader : failed to configure.`
	
	Rapl is not available on your machine. For the moment this is the only sensor plugin that can read CPU consumption. You can still disable it if your not interested in the CPU consumption, but only on the consumption of nvidia GPU. cf. [Sensor plugins](./user_guide/sensor_plugins.html)
	 
	 
 - `[Error][NVIDIA] NVML is not available.`
 
	 If you don't have an nvidia graphic card, the explanation is easy the plugin won't work. You will have to deactivate it in the configuration file. cf. [Sensor plugins](./user_guide/sensor_plugins.html).
	 
	 However, if you have a nvidia GPU make sure you have installed the Nvidia management library and that it matches the version of the nvidia drivers (sometimes it does not because.. nvidia). A quick test can be to run the command `nvidia-smi`. [Nvidia NVML](https://developer.nvidia.com/nvidia-management-library-nvml).


## B. vJoule is running but I don't have any consumption for my processes ?

Here are some common tracks for explanation : 

 - First you can check the result files of the whole system
   (`/etc/vjoule/results/cpu` for example). If they increment the
   sensor plugin and divider core are effectively running and
   working. Or run the command `vjoule top` to verify that vjoule is
   running and working properly.
   
 - The service runs at a given speed specified in the configuration file
   by the `freq` parameter. This specifies the number of iteration of the
   service per seconds (default being 1). This is an important
   information, if your application took less that a second to be
   executed, maybe the service just didn't have enough time to see it. To
   make sure your application will be seen by the service here a simple
   procedure to follow.
 
	 1. Configure vJoule service to monitor your cgroups, and create them (cf. [Cgroups](./getting_started/cgroups.html)).
 
	 2. Watch the file `/etc/vjoule/results/cpu` using iowatch, or inotify and wait for a modification. 
		 Here you can check wether the directory corresponding to your cgroups are correctly created.
		
	 3. Start your application and wait its end, or an event you have defined for your usecase.
 
	 4. Watch the file `/etc/vjoule/results/cpu` using iowatch or inotify to make sure the service has finished its iteration.
	 5. Read the result in the corresponding directory of your cgroups.
 
	 6. Delete your cgroups, and remove them from the vjoule service configuration.
 
	 Step 2 to 5 can be repeated multiple times, if you want to run multiple execution using the same cgroups.
 
 - If the cgroups you are watching appears and disappear during the execution (for example docker containers). 
   Maybe the divider core plugin has simply deleted the result file when the cgroup disappeared. You can remove that behavior by modifying the flag `delete-res` in the configuration file of the service (cf. [Divider core](./user_guide/divider_core.html)).

