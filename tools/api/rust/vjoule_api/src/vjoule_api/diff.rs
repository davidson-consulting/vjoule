
use std::time::{Duration};

use std::fmt;
use std::ops;

use crate::vjoule_api::perc::{ConsumptionPerc};

/**
 * A consumption diff is the difference between two consumption stamps
 */
#[derive(Debug, Copy, Clone)]
pub struct ConsumptionDiff {
    pub duration : Duration,
    pub pdu : f64,
    pub cpu : f64,
    pub gpu : f64,
    pub ram : f64
}


impl fmt::Display for ConsumptionDiff {
    
    fn fmt (&self, formatter : &mut fmt::Formatter)-> fmt::Result {
	    let r = format!("diff ({:?}, pdu: {:.2}J, cpu: {:.2}J, ram: {:.2}J, gpu: {:.2}J)", self.duration, self.pdu, self.cpu, self.ram, self.gpu);
	    formatter.write_str (&r)
    }	
}

impl ops::Rem <ConsumptionDiff> for ConsumptionDiff {
    type Output = ConsumptionPerc;
    
    fn rem (self, other : ConsumptionDiff)-> ConsumptionPerc {
	    ConsumptionPerc {
	        duration :  (self.duration.as_millis () as f64) / (other.duration.as_millis () as f64) * 100.0,
            pdu: if other.pdu != 0.0 {
                self.pdu / other.pdu * 100.0
            } else { 0.0 },
	        cpu : if other.cpu != 0.0 {
		        self.cpu / other.cpu * 100.0
	        } else { 0.0 },
	        ram : if other.ram != 0.0 {
		        self.ram / other.ram * 100.0
	        } else { 0.0 },
	        gpu : if other.gpu != 0.0 {
		        self.gpu / other.gpu * 100.0
	        } else {
		        0.0
	        }
	    }
    }
    
}

impl ops::Add <ConsumptionDiff> for ConsumptionDiff {
    type Output = ConsumptionDiff;


    fn add (self, other : ConsumptionDiff)-> ConsumptionDiff {
	    ConsumptionDiff {
	        duration : self.duration + other.duration,
            pdu : self.pdu + other.pdu,
	        cpu : self.cpu + other.cpu,
	        ram : self.ram + other.ram,
	        gpu : self.gpu + other.gpu
	    }
    }
    
}
