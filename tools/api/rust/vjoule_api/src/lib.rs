extern crate inotify;

use std::time::{SystemTime};
use std::fs;
use std::fs::File;
use std::io::Write;
use subprocess::Exec;

use inotify::{
    Inotify,
    WatchMask,
};


pub mod vjoule_api;
use crate::vjoule_api::stamp::{ConsumptionStamp};
use crate::vjoule_api::error::VJouleError;
use crate::vjoule_api::constants::{VJOULE_RES_PATH, VJOULE_PATH};

/**
 * Main element of the vjoule api
 */
pub struct VJouleAPI {
}

impl VJouleAPI {    

    /// Create and configure the vjoule api Create a this process_group refering to the pid of the
    /// current process (and all its threads / forks) To work properly the vjoule service has to be
    /// running
    ///
    /// # Example 
    /// ```
    /// let mut api = VJouleAPI::new ()?;
    /// let machine_conso = api.get_current_machine_consumption ()?;
    ///
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

	    Ok (VJouleAPI {})
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
    ///
    pub fn get_current_machine_consumption_no_force (&mut self)-> Result<ConsumptionStamp, VJouleError> {
	    let cpu_path = format!("{}/cpu", VJOULE_RES_PATH);
	    let ram_path = format!("{}/ram", VJOULE_RES_PATH);
	    let gpu_path = format!("{}/gpu", VJOULE_RES_PATH);
        let pdu_e_path = format!("{}/pdu_energy", VJOULE_RES_PATH);
        let pdu_p_path = format!("{}/pdu_power", VJOULE_RES_PATH);

        let cpu = self.read_value (&cpu_path[..]);
        let ram = self.read_value (&ram_path[..]);
	    let gpu = self.read_value (&gpu_path[..]);
        let pdu_e = self.read_value (&pdu_e_path[..]);
        let pdu_p = self.read_value (&pdu_p_path[..]);

	    Ok (ConsumptionStamp {
	        time : SystemTime::now (),
            pdu_watts : pdu_p,
            pdu : pdu_e,
	        cpu : cpu,
	        ram : ram,
	        gpu : gpu
	    })
    }

    ///
    /// # Returns
    /// the value read in the file at path 'file' if exists and contains a double, 0 otherwise
    ///
    fn read_value (&mut self, file : &str)-> f64 {
        match fs::read_to_string (&file) {
            Ok (content) => {
                match content[..content.len () - 1].parse::<f64> () {
                    Ok (result) => {
                        return result;
                    }
                    _ => {
                        return 0.0;
                    }
                }
            }
            _ => {
                return 0.0;
            }
        }

    }

    /// 
    /// Force an iteration on the vjoule service. This private function make sure the service has
    /// perform an iteration before reading the consumptions.
    /// 
    fn force_signal (&mut self) {
	    let mut inotif = Inotify::init ().expect ("Error while initializing inotify instance");
	    inotif.watches ().add (
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
