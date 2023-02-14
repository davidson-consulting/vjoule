extern crate inotify;

use std::time::{Duration, SystemTime};
use std::collections::HashMap;
use std::fs;
use std::path::Path;
use std::fs::File;
use std::io::Write;
use std::thread;
use std::process;
use subprocess::Exec;

use inotify::{
    Inotify,
    WatchMask,
};


pub mod vjoule_api;
use crate::vjoule_api::stamp::{ConsumptionStamp};
use crate::vjoule_api::process_group::{ProcessGroup};
use crate::vjoule_api::error::VJouleError;
use crate::vjoule_api::constants::{VJOULE_RES_PATH, VJOULE_PATH};
use crate::vjoule_api::utils::{read_lines};

/**
 * Main element of the vjoule api
 */
pub struct VJouleAPI {
    groups : HashMap<String, ProcessGroup>
}

impl VJouleAPI {    

    /// Create and configure the vjoule api Create a this process_group refering to the pid of the
    /// current process (and all its threads / forks) To work properly the vjoule service has to be
    /// running
    ///
    /// # Example 
    /// ```
    /// let mut api = VJouleAPI::new ()?;
    /// let pg = api.get_group ("this")?; // Process group created by default
    ///
    /// let pg_conso = pg.get_current_consumption ()?;
    /// let machine_conso = api.get_current_machine_consumption ()?;
    ///
    /// println!("Current pg consumption : {}", pg_conso);
    /// println!("Current machine consumption : {}", machine_conso);
    /// ```
    ///
    /// # Returns
    /// 
    /// - On success, the vjoule api
    /// - On error, a message explaining the error
    pub fn new ()-> Result <VJouleAPI, VJouleError> {
	
	let p = Exec::shell ("systemctl is-active --quiet vjoule_service.service").capture ().map_err (|_e| "Failed to execute bash command")?;
	match p.exit_status {
	    subprocess::ExitStatus::Exited (0) => {}
	    _ => {
		return Err ("vJoule service is not started. sudo systemctl start vjoule_service.service");
	    }
	}
			
	let lines = read_lines(format!("{}/cgroups", VJOULE_PATH));
	let mut found = false;
	for line in &lines {
	    if line == "vjoule_api.slice/*" {
		found = true;
		break;
	    }
	}

	if !found {
	    let file = File::create (format!("{}/cgroups", VJOULE_PATH));
	    match file {		
		Ok (mut f) => {
		    for l in &lines {
			f.write_all (l.as_bytes ()).map_err (|_e| "Failed to write in 'cgroups' file")?;
			f.write_all (b"\n").map_err (|_e| "Failed to write in 'cgroups' file")?;
		    }
		    
		    f.write_all (b"vjoule_api.slice/*\n").map_err (|_e| "Failed to write in 'cgroups' file")?;
		    f.sync_all ().map_err (|_e| "Failed to write in 'cgroups' file")?;
		}
		_ => {
		    return Err ("Failed to write in 'cgroups' file");
		}
	    }
	}

	let create = Exec::shell (format!("vjoule_cgutils attach vjoule_api.slice/this_{0} {0}", process::id ())).capture ().map_err (|_e| "Failed to run attach command")?;
	match create.exit_status {
	    subprocess::ExitStatus::Exited (0) => {}
	    _ => {
		return Err ("Failed to attach process to cgroup. make sure the current user is in group 'vjoule'.")
	    }
	}

	let mut h = HashMap::new ();
	let gp = ProcessGroup::new (&format!("this_{}", process::id ())[..], false, false);
	h.insert (format!("this_{}", process::id ()).clone (), gp);

	let gp2 = ProcessGroup::new (&format!("this_{}", process::id ())[..], false, false);
	h.insert (String::from("this"), gp2);	
	
	Ok (VJouleAPI { groups : h })	    
    }

    ///
    /// # Returns
    /// The consumption of the machine since the start of the vjoule service
    ///
    /// # Information
    /// 
    /// This function make sure the vjoule service has written a consumption before returning. It
    /// forces the service to perform an iteration to avoid waiting too long, but it can take a bit
    /// of time (some milliseconds).
    ///
    pub fn get_current_machine_consumption (&mut self)-> Result<ConsumptionStamp, VJouleError> {
	self.force_signal ();
	self.get_current_machine_consumption_no_force ()
    }

    ///
    /// # Returns
    /// The consumption of the machine since the start of the vjoule service
    ///
    /// # Warning
    /// 
    /// Should be used iif another get_current_consumption was made just before (e.g. for a process group).
    ///
    pub fn get_current_machine_consumption_no_force (&mut self)-> Result<ConsumptionStamp, VJouleError> {
	let cpu_path = format!("{}/cpu", VJOULE_RES_PATH);
	let ram_path = format!("{}/ram", VJOULE_RES_PATH);
	let gpu_path = format!("{}/gpu", VJOULE_RES_PATH);

	let cpu_s = fs::read_to_string(&cpu_path[..]).map_err (|_e| "Failed to read cpu file")?; 
	let cpu = cpu_s [..cpu_s.len () - 1].parse::<f64> ().map_err (|_e| "Failed to read cpu file")?;

	let ram_s = fs::read_to_string(&ram_path[..]).map_err (|_e| "Failed to read ram file")?; 
	let ram = ram_s [..ram_s.len () - 1].parse::<f64> ().map_err (|_e| "Failed to read ram file")?;

	let gpu_s = fs::read_to_string(&gpu_path[..]).map_err (|_e| "Failed to read gpu file")?; 
	let gpu = gpu_s [..gpu_s.len () - 1].parse::<f64> ().map_err (|_e| "Failed to read gpu file")?; 

	Ok (ConsumptionStamp {
	    time : SystemTime::now (),
	    cpu : cpu,
	    ram : ram,
	    gpu : gpu
	})
    }

    ///
    ///
    /// # Parameters
    ///    - name: the name of the cgroup / process group to retreive (for example "this", or "my.slice/my_cgroup")
    /// 
    /// # Returns
    /// 
    /// A process group (or cgroup) that already exists on the machine
    ///
    /// # Information
    /// 
    /// Groups created by the api (cf. create_group) are placed in the slice 'vjoule_api.slice/',
    /// however there name is only the sub cgroup for example, the group 'vjoule_api.slice/my_group'
    /// will only be named 'my_group' in the api known cgroups map.  Using the full name
    /// 'vjoule_api.slice/my_group' will also work but will cause unnecessary additionnal
    /// computation.
    ///
    /// Cgroups do not need to be watched by the vjoule service before calling this
    /// function, the api will make sure they are.
    /// 
    pub fn get_group (&mut self, name : &str)-> Result <ProcessGroup, VJouleError> {
	let cgroup_name = String::from (&name[..]);
	match self.groups.get (&cgroup_name) {
	    Some (p) => { Ok (p.clone ()) }
	    _ => {
		let lines = read_lines(format!("{}/cgroups", VJOULE_PATH));
		let mut found = false;
		for line in &lines {
		    if line == name {
			found = true;
			break;
		    }
		}

		if !found {
		    let file = File::create (format!("{}/cgroups", VJOULE_PATH));
		    match file {
			Ok (mut f) => {
			    for l in &lines {
				f.write_all (l.as_bytes ()).map_err (|_e| "Failed to write in 'cgroups' file")?;
				f.write_all (b"\n").map_err (|_e| "Failed to write in 'cgroups' file")?;
			    }
			    
			    f.write_all (name.as_bytes ()).map_err (|_e| "Failed to write in 'cgroups' file")?;
			    f.write_all (b"\n").map_err (|_e| "Failed to write in 'cgroups' file")?;
			    f.sync_all ().map_err (|_e| "Failed to write in 'cgroups' file")?
			}
			_ => {
			    return Err ("Failed to write in 'cgroups' file");
			}
		    }
		}

		self.force_signal ();
		for _i in 0 .. 2 {
		    if Path::new (&format!("{}/results/{}/cpu", VJOULE_PATH, name)).exists () {
			let ret = ProcessGroup::new (name, true, !found);
			self.groups.insert (cgroup_name.clone (), ret);

			return Ok (self.groups.get (name).expect ("???").clone ());
		    }
		    thread::sleep (Duration::from_millis (10));
		}
		
		Err ("Group not found")
	    }
	}
    }

    ///
    /// Create a new process group in the slice 'vjoule_api.slice'.
    /// After the creation the pids listed in 'pids' are attached to the cgroup.
    /// The API makes sure the newly created cgroup is watched by the vjoule service.
    ///
    /// # Parameters
    /// - name: the name of the group to create (e.g. "my_group")
    /// - pids: a list of pids to attach to the group
    /// 
    /// # Information
    /// 
    /// The created cgroup is placed in the 'vjoule_api.slice', but is refered only by its name in
    /// the API.  For example the process group 'vjoule_api.slice/my_group' will be known as
    /// 'my_group' by the api.  Only the short name should be passed to the different functions of
    /// the API (i.e. create_group and get_group).
    ///
    /// If the group already exists, the function won't fail, as a matter of fact calling this
    /// function multiple times can have an interest as it can be used to attach new pids to an
    /// existing process group.
    ///
    /// # Example
    /// ```
    /// let mut api = VJouleApi::new ();
    /// let mut pg = api.create_group ("my_group", &[123, 569]);
    ///
    /// // attach new pids to the process group
    /// pg = ap.create_group ("my_group", &[786, 902]);
    ///
    /// // even if the prefered way is using the attach function
    /// pg.attach (&[45, 897]);
    ///
    /// ```
    /// 
    pub fn create_group (&mut self, name : &str, pids : &[u64])-> Result<ProcessGroup, VJouleError> {
	let mut owned : String = format!("vjoule_cgutils attach vjoule_api.slice/{}", name);
	for i in pids {
	    let j = format!(" {}", i);
	    owned.push_str (&j[..]);
	}
	
	let create = Exec::shell (&owned[..]).capture ().map_err (|_e| "Failed to run attach command")?;
	match create.exit_status {
	    subprocess::ExitStatus::Exited (0) => {}
	    _ => {
		return Err ("Failed to attach processes to cgroup. make sure the current user is in group 'vjoule'.")
	    }
	}

	let ret = ProcessGroup::new (name, false, true);
	self.groups.insert (String::from (name), ret);

	return Ok (self.groups.get (name).expect ("???").clone ());
    }
        
    /// 
    /// Force an iteration on the vjoule service. This private function make sure the service has
    /// perform an iteration before reading the consumptions.
    /// 
    fn force_signal (&mut self) {
	let mut inotif = Inotify::init ().expect ("Error while initializing inotify instance");
	inotif.add_watch (
	    format!("{}/cpu", VJOULE_RES_PATH),
	    WatchMask::MODIFY
	).expect ("Failed to add watch");

	for _i in 0 .. 2 {
	    let file = File::create (format!("{}/signal", VJOULE_PATH));
	    match file {
		Ok (mut f) => {
		    f.write_all (b"1").expect ("Failed to write in signal file");
		    f.sync_all ().expect ("Failed to close signal file");
		}
		_ => {
		}
	    }
	    
	    let mut buffer = [0 ; 1024];
	    inotif.read_events_blocking (&mut buffer)
		.expect ("Error while reading events");
	}
    }
    
}


impl Drop for VJouleAPI {

    ///
    /// Clean every elements created by the API on the system.
    /// This function is really important and should not be ignored.
    /// 
    fn drop (&mut self)  {
	for (key, value) in &self.groups {
	    match value.dispose () {
		Ok (_) => {}
		Err (s) => {
		    println!("Dropping error '{}' : {}", key, s);
		}
	    }
	}
	
	self.groups.clear ();	
    }

}
