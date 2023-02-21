
use std::time::{SystemTime};
use std::fs;
use std::fs::File;
use std::io::Write;
use subprocess::Exec;

use crate::vjoule_api::stamp::{ConsumptionStamp};
use crate::vjoule_api::constants::{VJOULE_RES_PATH, VJOULE_PATH};
use crate::vjoule_api::error::{VJouleError};
use crate::vjoule_api::utils::{read_lines};


use inotify::{
    Inotify,
    WatchMask,
};


#[derive(Debug, Clone)]
pub struct ProcessGroup {
    name : String,
    result_loc : String,
    no_slice : bool,
    did_add : bool
}

impl ProcessGroup {

    ///
    /// Create a new process group
    /// # Warning
    ///
    /// This function should only be could using an API.
    /// 
    pub fn new (name : &str, no_slice : bool, did_add : bool)-> ProcessGroup {
	let result_loc = if no_slice {
	    format!("{}", name)
	} else {
	    format!("vjoule_api.slice/{}", name)
	};

	
	ProcessGroup { name : String::from (&name[..]), result_loc : result_loc, no_slice : no_slice, did_add : did_add }
    }

    ///
    /// # Returns
    ///
    /// The estimation of the consumption of the process group since the moment it was watched by the vjoule service.
    ///
    /// # Information
    /// 
    /// This function make sure the vjoule service has written a consumption before returning. It
    /// forces the service to perform an iteration to avoid waiting too long, but it can take a bit
    /// of time (some milliseconds).
    ///
    pub fn get_current_consumption (&mut self)-> Result<ConsumptionStamp, VJouleError> {
	self.force_signal ();
	self.get_current_consumption_no_force ()
    }
    
    ///
    /// 
    /// # Returns
    ///
    /// The estimation of the consumption of the process group since
    /// the moment it was watched by the vjoule service.
    /// 
    /// # Warning
    /// 
    /// Should be used iif another get_current_consumption was made just before (e.g. for a machine consumption).
    ///
    pub fn get_current_consumption_no_force (&mut self)-> Result<ConsumptionStamp, VJouleError> {	
	let cpu_path = format!("{}{}/cpu", VJOULE_RES_PATH, self.result_loc);
	let ram_path = format!("{}{}/ram", VJOULE_RES_PATH, self.result_loc);
	let gpu_path = format!("{}{}/gpu", VJOULE_RES_PATH, self.result_loc);
	
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
    /// Attach pids to the process group
    ///
    /// # Warning
    ///
    /// A pid can be attached to only on cgroup at a time, so if it was in a cgroup, it will be detached from it
    /// 
    pub fn attach (&mut self, pids : &[u64])-> Result <(), VJouleError> {
	let mut owned : String = if !self.no_slice {
	    format!("vjoule_cgutils attach vjoule_api.slice/{}", self.name)
	} else {
	    format!("vjoule_cgutils attach {}", self.name)
	};
	
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

	Ok (())
    }

    ///
    /// Detach pids from the processe group
    ///
    /// # Warning
    /// 
    /// Detached processes will be attached to no cgroup afterwards
    /// 
    pub fn detach (&mut self, pids : &[u64])-> Result<(), VJouleError> {
	let mut owned : String = format!("vjoule_cgutils detach");	
	for i in pids {
	    let j = format!(" {}", i);
	    owned.push_str (&j[..]);
	}
	
	let create = Exec::shell (&owned[..]).capture ().map_err (|_e| "Failed to run detach command")?;
	match create.exit_status {
	    subprocess::ExitStatus::Exited (0) => {}
	    _ => {
		return Err ("Failed to detach processes from cgroup. make sure the current user is in group 'vjoule'.")
	    }
	}

	Ok (())
    }
    
    fn force_signal (&mut self) {
	let mut inotif = Inotify::init ().expect ("Error while initializing inotify instance");
	inotif.add_watch (
	    format!("{}{}/cpu", VJOULE_RES_PATH, self.result_loc),
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

    ///
    /// Clear everything created for the process group on the system
    /// 
    /// # Warning
    ///
    /// This function is called when the Vjoule API is dropped, it is not necessary to call it manually, and maybe it should not be done as it will invalidate some elements that may be used by the API in the future
    /// 
    pub fn dispose (&self) -> Result<(), VJouleError> {
	if !self.no_slice {
	    let del = Exec::shell (format!("vjoule_cgutils del vjoule_api.slice/{}", self.name)).capture ().map_err (|_e| "Failed to run 'vjoule_cgutils del' command")?;
	    match del.exit_status {
		subprocess::ExitStatus::Exited (0) => { return Ok (()); }
		_ => {
		    return Err ("Failed to remove cgroup");
		}
	    }
	} else if self.did_add {
	    let lines = read_lines(format!("{}/cgroups", VJOULE_PATH));
	    let file = File::create (format!("{}/cgroups", VJOULE_PATH));
	    match file {
		Ok (mut f) => {
		    for l in lines {			
			if l != self.name {
			    f.write_all (l.as_bytes ()).map_err (|_e| "Failed to write in 'cgroups' file")?;
			    f.write_all (b"\n").map_err (|_e| "Failed to write in 'cgroups' file")?;
			}
		    }
		    f.sync_all ().map_err (|_e| "Failed to write in 'cgroups' file")?;
		}
		_ => {
		    return Err ("Failed to write in 'cgroups' file");
		}
	    }

	}
	
	Ok (())
    }

    
}
